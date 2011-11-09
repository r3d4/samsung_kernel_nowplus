/*
 *  Universal accessory detect class
 *
 *
 *  You may use this code as per GPL version 2
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include "./accessory.h"
struct class *accessory_class;


static struct device_attribute accessory_attrs[];

static ssize_t accessory_show_property(struct device *dev,
					  struct device_attribute *attr,
					  char *buf) {
	ssize_t ret;
	struct accessory *acy = dev_get_drvdata(dev);
	const ptrdiff_t off = attr - accessory_attrs;
	union accessory_propval value;

	ret = acy->get_property(acy, off, &value);

	if (ret < 0) {
		if (ret != -ENODEV)
			dev_err(dev, "driver failed to report `%s' property\n",
				attr->attr.name);
		return ret;
	}

	return sprintf(buf, "%d\n", value.intval);
}

#define ACCESSORY_ATTR(_name)					\
{									\
	.attr = { .name = #_name, .mode = 0444, .owner = THIS_MODULE },	\
	.show = accessory_show_property,				\
	.store = NULL,							\
}

/* Must be in the same order as ACCESSORY_PROP_* */
static struct device_attribute accessory_attrs[] = {
	/* Properties of type `int' */
	ACCESSORY_ATTR(online),

	/* Properties of type `const char *' */
	ACCESSORY_ATTR(model_name),
	ACCESSORY_ATTR(manufacturer),
	ACCESSORY_ATTR(serial_number),
};
static ssize_t accessory_show_static_attrs(struct device *dev,
					      struct device_attribute *attr,
					      char *buf) {
	static char *type_text[] = { "headset", "carkit"};
	struct accessory *acy = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", type_text[acy->type]);
}

static struct device_attribute accessory_static_attrs[] = {
	__ATTR(type, 0444, accessory_show_static_attrs, NULL),
};

static int accessory_create_attrs(struct accessory *acy)
{
	int rc = 0;
	int i, j;

	for (i = 0; i < ARRAY_SIZE(accessory_static_attrs); i++) {
		rc = device_create_file(acy->dev,
			    &accessory_static_attrs[i]);
		if (rc)
			goto statics_failed;
	}

	for (j = 0; j < acy->num_properties; j++) {
		rc = device_create_file(acy->dev,
			    &accessory_attrs[acy->properties[j]]);
		if (rc)
			goto dynamics_failed;
	}

	goto succeed;

dynamics_failed:
	while (j--)
		device_remove_file(acy->dev,
			   &accessory_attrs[acy->properties[j]]);
statics_failed:
	while (i--)
		device_remove_file(acy->dev, &accessory_static_attrs[i]);
succeed:
	return rc;
}

static void accessory_remove_attrs(struct accessory *acy)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(accessory_static_attrs); i++)
		device_remove_file(acy->dev, &accessory_static_attrs[i]);

	for (i = 0; i < acy->num_properties; i++)
		device_remove_file(acy->dev,
			    &accessory_attrs[acy->properties[i]]);
}

static int accessory_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	return 0;
}


void accessory_changed(struct accessory *acy)
{
	dev_dbg(acy->dev, "%s\n", __FUNCTION__);

	kobject_uevent(&acy->dev->kobj, KOBJ_CHANGE);
}


static int __accessory_match_name(struct device *dev, void *data)
{
	char *acy_name = (char *)data;
	struct accessory *eacy = dev_get_drvdata(dev);

	if (!strcmp(eacy->name, acy_name))
		return 1;
	else
		return 0;
}

struct accessory* accessory_get_by_name(char* name)
{
	struct device *dev;
	struct accessory *acy = NULL;

	#if 0
	dev = class_find_device(accessory_class, name,
						__accessory_match_name);
	if( dev )
		acy = dev_get_drvdata(dev);
	#endif

	return acy;
}

int accessory_register(struct device *parent, struct accessory *acy)
{
	int rc = 0;

	acy->dev = device_create(accessory_class, parent, 0,
				 "%s", acy->name);
	if (IS_ERR(acy->dev)) {
		rc = PTR_ERR(acy->dev);
		goto dev_create_failed;
	}

	dev_set_drvdata(acy->dev, acy);

	rc = accessory_create_attrs(acy);
	if (rc)
		goto create_attrs_failed;

	accessory_changed(acy);

	goto success;

	accessory_remove_attrs(acy);
create_attrs_failed:
	device_unregister(acy->dev);
dev_create_failed:
success:
	return rc;
}

void accessory_unregister(struct accessory *acy)
{
	accessory_remove_attrs(acy);
	device_unregister(acy->dev);
}

static int __init accessory_class_init(void)
{
	accessory_class = class_create(THIS_MODULE, "accessory");

	if (IS_ERR(accessory_class))
		return PTR_ERR(accessory_class);

	accessory_class->dev_uevent = accessory_uevent;

	return 0;
}

static void __exit accessory_class_exit(void)
{
	class_destroy(accessory_class);
}

EXPORT_SYMBOL_GPL(accessory_changed);
EXPORT_SYMBOL_GPL(accessory_get_by_name);
EXPORT_SYMBOL_GPL(accessory_register);
EXPORT_SYMBOL_GPL(accessory_unregister);

/* exported for the APM Power driver, APM emulation */
EXPORT_SYMBOL_GPL(accessory_class);

subsys_initcall(accessory_class_init);
module_exit(accessory_class_exit);

MODULE_DESCRIPTION("Universal accessory monitor class");
MODULE_LICENSE("GPL");


