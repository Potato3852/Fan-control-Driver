#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/platform_data/x86/asus-wmi.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nikita");
MODULE_DESCRIPTION("ASUS Zenbook Fan Control Driver");
MODULE_VERSION("0.1.1");
MODULE_IMPORT_NS("ASUS_WMI");

/**
 * @def ASUS_WMI_DEVID_THROTTLE_FAN_POLICY
 * @brief WMI device ID for the throttle thermal policy, VIVO variant (0x00110019).
 */
#define ASUS_WMI_DEVID_THROTTLE_FAN_POLICY 0x00110019

/**
 * @brief sysfs fan_mode -> EC policy value.
 */
static const u8 fan_mode_to_policy[] = {
    [0] = 1, /* quiet       -> silent    */
    [1] = 0, /* balanced    -> default   */
    [2] = 2, /* performance -> overboost */
};

/**
 * @brief EC policy value -> sysfs fan_mode.
 */
static const u8 policy_to_fan_mode[] = {
    [0] = 1, /* default   -> balanced    */
    [1] = 0, /* silent    -> quiet       */
    [2] = 2, /* overboost -> performance */
};

static int current_fan_mode = 1;
static DEFINE_MUTEX(fan_mode_lock);
static struct platform_device *zenbook_pdev;

/**
 * @brief Reads the active policy directly from the EC.
 * @return sysfs fan_mode (0..2), or a negative error code.
 */
static int get_fan_profile(void) {
    u32 retval = 0;
    int err, policy;

    err = asus_wmi_get_devstate_dsts(ASUS_WMI_DEVID_THROTTLE_FAN_POLICY, &retval);
    if (err) {
        return err;
    }

    policy = retval & 0x3;
    if (policy > 2) {
        return -EIO;
    }

    return policy_to_fan_mode[policy];
}

/**
 * @brief Sets the thermal policy profile via the ASUS WMI interface.
 * @return 0 on success, or a negative error code on failure.
 */
static int set_fan_profile(int mode) {
    u32 retval = 0;
    int err;

    if (mode < 0 || mode > 2) {
        return -EINVAL;
    }

    err = asus_wmi_set_devstate(ASUS_WMI_DEVID_THROTTLE_FAN_POLICY, fan_mode_to_policy[mode], &retval);
    if (err) {
        pr_err("zenbook_fan: Failed to set thermal policy (mode=%d, err=%d)\n", mode, err);
        return err;
    }

    pr_info("zenbook_fan: Profile set to %d (EC retval: %u)\n", mode, retval);
    return 0;
}

/**
 * @brief sysfs show callback for the current fan profile.
 */
static ssize_t fan_mode_show(struct device *dev, struct device_attribute *attr, char *buf) {
    int mode;

    mutex_lock(&fan_mode_lock);
    mode = get_fan_profile();
    if (mode < 0) {
        mode = current_fan_mode;
    }
    mutex_unlock(&fan_mode_lock);

    return sysfs_emit(buf, "%d\n", mode);
}

/**
 * @brief sysfs store callback for setting a new fan profile.
 * @param buf "0" (quiet), "1" (balanced) or "2" (performance)
 */
static ssize_t fan_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    int val, res;

    res = kstrtoint(buf, 10, &val);
    if (res < 0) {
        return res;
    }

    if (val < 0 || val > 2) {
        return -EINVAL;
    }

    mutex_lock(&fan_mode_lock);
    res = set_fan_profile(val);
    if (res == 0) {
        current_fan_mode = val;
    }
    mutex_unlock(&fan_mode_lock);

    return res < 0 ? res : count;
}

static DEVICE_ATTR_RW(fan_mode);

static struct attribute *zenbook_attrs[] = {
    &dev_attr_fan_mode.attr,
    NULL,
};

static const struct attribute_group zenbook_attr_group = {
    .attrs = zenbook_attrs,
};

/**
 * @brief Module initialization routine.
 */
static int __init zenbook_init(void) {
    int ret, mode;

    pr_info("zenbook_fan: Initializing Thermal Policy driver...\n");

    /* Verify the device is present and readable before exposing sysfs. */
    mode = get_fan_profile();
    if (mode < 0) {
        pr_err("zenbook_fan: Throttle thermal policy device not present or unreadable (err=%d)\n", mode);
        return mode;
    }

    zenbook_pdev = platform_device_register_simple("zenbook_fan", -1, NULL, 0);
    if (IS_ERR(zenbook_pdev)) {
        return PTR_ERR(zenbook_pdev);
    }

    ret = sysfs_create_group(&zenbook_pdev->dev.kobj, &zenbook_attr_group);
    if (ret) {
        platform_device_unregister(zenbook_pdev);
        return ret;
    }
    current_fan_mode = mode;

    pr_info("zenbook_fan: Registered successfully! Current mode: %d (0=quiet, 1=balanced, 2=performance)\n", mode);
    return 0;
}

/**
 * @brief Module cleanup routine.
 */
static void __exit zenbook_exit(void) {
    pr_info("zenbook_fan: Unloading module...\n");

    if (zenbook_pdev) {
        sysfs_remove_group(&zenbook_pdev->dev.kobj, &zenbook_attr_group);
        platform_device_unregister(zenbook_pdev);
    }

    pr_info("zenbook_fan: Module unloaded.\n");
}

module_init(zenbook_init);
module_exit(zenbook_exit);