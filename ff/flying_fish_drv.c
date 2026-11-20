#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Nagao");
MODULE_DESCRIPTION("Platform Driver com IRQ para Sensor Flying Fish");

struct ff_data {
    struct gpio_desc *sensor_gpio;
    struct gpio_desc *buzzer_gpio;
    int irq_num;
};

// Rotina de Interrupção (Executada instantaneamente pelo hardware ao mudar o pino)
static irqreturn_t sensor_irq_handler(int irq, void *dev_id) {
    struct ff_data *data = (struct ff_data *)dev_id;
    
    // Como definimos ACTIVE_LOW no .dts, gpiod_get_value() já retorna 1 quando detecta obstáculo!
    int obstaculo_detectado = gpiod_get_value(data->sensor_gpio);

    if (obstaculo_detectado) {
        gpiod_set_value(data->buzzer_gpio, 1); // Liga o Buzzer
        pr_info("flying_fish: [ALERTA] Mao detectada! Buzzer LIGADO.\n");
    } else {
        gpiod_set_value(data->buzzer_gpio, 0); // Desliga o Buzzer
        pr_info("flying_fish: [LIVRE] Area limpa. Buzzer DESLIGADO.\n");
    }

    return IRQ_HANDLED;
}

// Função Probe: Chamada pelo Kernel quando encontra "nagao,flying-fish" na Device Tree
static int ff_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct ff_data *data;
    int ret;

    pr_info("flying_fish: Hardware encontrado na Device Tree! Inicializando driver...\n");

    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    // 1. Requisita os pinos definidos em "sensor-gpios" e "buzzer-gpios" no .dts
    data->sensor_gpio = devm_gpiod_get(dev, "sensor", GPIOD_IN);
    if (IS_ERR(data->sensor_gpio)) {
        dev_err(dev, "Falha ao obter sensor-gpios\n");
        return PTR_ERR(data->sensor_gpio);
    }

    data->buzzer_gpio = devm_gpiod_get(dev, "buzzer", GPIOD_OUT_LOW);
    if (IS_ERR(data->buzzer_gpio)) {
        dev_err(dev, "Falha ao obter buzzer-gpios\n");
        return PTR_ERR(data->buzzer_gpio);
    }

    // 2. Descobre qual número de interrupção (IRQ) o hardware atribuiu ao pino do sensor
    data->irq_num = gpiod_to_irq(data->sensor_gpio);
    if (data->irq_num < 0) {
        dev_err(dev, "Falha ao mapear GPIO para IRQ\n");
        return data->irq_num;
    }

    // 3. Registra a interrupção para disparar tanto na aproximação quanto no afastamento
    ret = devm_request_irq(dev, data->irq_num, sensor_irq_handler,
                           IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                           "flying_fish_irq", data);
    if (ret) {
        dev_err(dev, "Falha ao registrar IRQ %d\n", data->irq_num);
        return ret;
    }

    platform_set_drvdata(pdev, data);
    pr_info("flying_fish: Driver ativo! IRQ %d pronta.\n", data->irq_num);
    return 0;
}

static void ff_remove(struct platform_device *pdev) {
    struct ff_data *data = platform_get_drvdata(pdev);
    gpiod_set_value(data->buzzer_gpio, 0); // Garante que o buzzer desligue ao remover o módulo
    pr_info("flying_fish: Driver removido com seguranca.\n");
}

// Tabela de casamento (Matching) com a Device Tree
static const struct of_device_id ff_of_ids[] = {
    { .compatible = "nagao,flying-fish" },
    { /* sentinela */ }
};
MODULE_DEVICE_TABLE(of, ff_of_ids);

static struct platform_driver ff_driver = {
    .probe  = ff_probe,
    .remove = ff_remove,
    .driver = {
        .name           = "flying_fish_driver",
        .of_match_table = ff_of_ids,
    },
};

module_platform_driver(ff_driver);
