#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nikita");
MODULE_DESCRIPTION("ASUS Zenbook Fan Control Driver");

static struct platform_device *zenbook_pdev;

static int current_fan_mode = 0;

static ssize_t fan_mode_show(struct device *dev, struct device_attribute *attr, char *buf) {
    return sysfs_emit(buf, "%d\n", current_fan_mode);;
}

static ssize_t fan_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    int val;
    int res;

    res = kstrtoint(buf, 10, &val);
    if (res < 0) {
        return res;
    }

    if (val < 0 || val > 2) {
        pr_warn("Zenbook Fan Driver: Invalid mode %d! Use 0, 1, or 2.\n", val);
        return -EINVAL;
    }

    current_fan_mode = val;

    pr_info("Zenbook Fan Driver: Switched fan mode to %d\n", current_fan_mode);

    return count;
}

static DEVICE_ATTR_RW(fan_mode);

static struct attribute *zenbook_attrs[] = {
    &dev_attr_fan_mode.attr,
    NULL,
};

static const struct attribute_group zenbook_attr_group = {
    .attrs = zenbook_attrs,
};

static int __init zenbook_init(void) {
    int ret;
    pr_info("Zenbook Fan Driver: Initializing...\n");

    zenbook_pdev = platform_device_register_simple("zenbook_fan", -1, NULL, 0);
    if (IS_ERR(zenbook_pdev)) {
        pr_err("Zenbook Fan Driver: Failed to register device!\n");
        return PTR_ERR(zenbook_pdev);
    }

    ret = sysfs_create_group(&zenbook_pdev->dev.kobj, &zenbook_attr_group);
    if (ret) {
        pr_err("Zenbook Fan Driver: Failed to create sysfs group!\n");
        platform_device_unregister(zenbook_pdev);
        return ret;
    }

    pr_info("Zenbook Fan Driver: Device registered successfully with sysfs!\n");
    return 0;
}

static void __exit zenbook_exit(void) {
    pr_info("Zenbook Fan Driver: Unloading module...\n");

    if (zenbook_pdev) {
        sysfs_remove_group(&zenbook_pdev->dev.kobj, &zenbook_attr_group);
        platform_device_unregister(zenbook_pdev);
    }

    pr_info("Zenbook Fan Driver: Module unloaded.\n");
}

module_init(zenbook_init);
module_exit(zenbook_exit);