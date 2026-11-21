#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Nagao");
MODULE_DESCRIPTION("Platform Driver Sincronizado com Delayed Workqueue para Flying Fish");

struct ff_data {
    struct gpio_desc *sensor_gpio;
    struct gpio_desc *buzzer_gpio;
    int irq_num;
    struct delayed_work debounce_work;
};

// Esta função roda 15ms APÓS o último ruído elétrico, garantindo leitura estável
static void ff_work_handler(struct work_struct *work) {
    struct ff_data *data = container_of(work, struct ff_data, debounce_work.work);
    int ativo = gpiod_get_value(data->sensor_gpio);

    gpiod_set_value(data->buzzer_gpio, ativo);
    pr_info("flying_fish: [SINCRONIZADO] Sensor=%d -> Buzzer %s\n",
            ativo, ativo ? "LIGADO" : "DESLIGADO");
}

// A interrupção apenas reinicia o cronômetro de 15ms a cada borda detectada
static irqreturn_t sensor_irq_handler(int irq, void *dev_id) {
    struct ff_data *data = (struct ff_data *)dev_id;

    // mod_delayed_work cancela qualquer leitura pendente e empurra para 15ms à frente
    mod_delayed_work(system_wq, &data->debounce_work, msecs_to_jiffies(15));
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

    // Inicializa a fila de trabalho atrasada (Workqueue)
    INIT_DELAYED_WORK(&data->debounce_work, ff_work_handler);

    data->irq_num = gpiod_to_irq(data->sensor_gpio);
    if (data->irq_num < 0)
        return data->irq_num;

    ret = devm_request_irq(dev, data->irq_num, sensor_irq_handler,
                           IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                           "flying_fish_irq", data);
    if (ret)
        return ret;

    platform_set_drvdata(pdev, data);

    // Sincroniza o estado inicial na carga do módulo
    gpiod_set_value(data->buzzer_gpio, gpiod_get_value(data->sensor_gpio));
    pr_info("flying_fish: Driver sincronizado ativo na IRQ %d!\n", data->irq_num);
    return 0;
}

static void ff_remove(struct platform_device *pdev) {
    struct ff_data *data = platform_get_drvdata(pdev);
    cancel_delayed_work_sync(&data->debounce_work); // Garante que não há tarefa pendente
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
