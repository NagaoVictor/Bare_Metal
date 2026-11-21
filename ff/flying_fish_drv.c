#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Nagao");
MODULE_DESCRIPTION("Platform Driver com IRQ e Debounce para Sensor Flying Fish");

struct ff_data {
    struct gpio_desc *sensor_gpio;
    struct gpio_desc *buzzer_gpio;
    int irq_num;
    u64 last_irq_ns;
};

static void atualizar_buzzer(struct ff_data *data) {
    // gpiod_get_value() já aplica a inversão do ACTIVE_LOW definido no .dts
    int ativo = gpiod_get_value(data->sensor_gpio);
    // gpiod_get_raw_value() lê a tensão física real (0 = 0V, 1 = 3.3V)
    int fisico = gpiod_get_raw_value(data->sensor_gpio);

    gpiod_set_value(data->buzzer_gpio, ativo);

    pr_info("flying_fish: [IRQ] Tensao fisica=%d | Logico ativo=%d -> Buzzer %s\n",
            fisico, ativo, ativo ? "LIGADO" : "DESLIGADO");
}

static irqreturn_t sensor_irq_handler(int irq, void *dev_id) {
    struct ff_data *data = (struct ff_data *)dev_id;
    u64 agora = ktime_get_ns();

    // Debounce de software de 20 ms (20.000.000 ns) para filtrar oscilação do LM393
    if (agora - data->last_irq_ns < 20000000ULL)
        return IRQ_HANDLED;

    data->last_irq_ns = agora;
    atualizar_buzzer(data);

    return IRQ_HANDLED;
}

static int ff_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct ff_data *data;
    int ret;

    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->sensor_gpio = devm_gpiod_get(dev, "sensor", GPIOD_IN);
    if (IS_ERR(data->sensor_gpio))
        return PTR_ERR(data->sensor_gpio);

    data->buzzer_gpio = devm_gpiod_get(dev, "buzzer", GPIOD_OUT_LOW);
    if (IS_ERR(data->buzzer_gpio))
        return PTR_ERR(data->buzzer_gpio);

    data->irq_num = gpiod_to_irq(data->sensor_gpio);
    if (data->irq_num < 0)
        return data->irq_num;

    ret = devm_request_irq(dev, data->irq_num, sensor_irq_handler,
                           IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                           "flying_fish_irq", data);
    if (ret)
        return ret;

    platform_set_drvdata(pdev, data);

    // Sincroniza o estado inicial imediatamente ao carregar o driver
    atualizar_buzzer(data);
    pr_info("flying_fish: Driver pronto na IRQ %d!\n", data->irq_num);
    return 0;
}

static void ff_remove(struct platform_device *pdev) {
    struct ff_data *data = platform_get_drvdata(pdev);
    gpiod_set_value(data->buzzer_gpio, 0);
    pr_info("flying_fish: Driver removido.\n");
}

static const struct of_device_id ff_of_ids[] = {
    { .compatible = "nagao,flying-fish" },
    { }
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
