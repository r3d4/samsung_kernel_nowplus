/*
 *  Universal accessory monitor class
 *
 *
 *  You may use this code as per GPL version 2
 */

#ifndef __ACCESSORY_H__
#define __ACCESSORY_H__

#include <linux/device.h>

enum accessory_property {
	/* Properties of type `int' */
	ACCESSORY_PROP_ONLINE = 0,


	/* Properties of type `const char *' */
	ACCESSORY_PROP_MODEL_NAME,
	ACCESSORY_PROP_MANUFACTURER,
	ACCESSORY_PROP_SERIAL_NUMBER,
};

enum accessory_type {
	ACCESSORY_TYPE_HEADSET = 0,
	ACCESSORY_TYPE_CARKIT,
};

union accessory_propval {
	int intval;
	const char *strval;
};

struct accessory {
	const char *name;
	enum accessory_type type;
	enum accessory_property *properties;
	size_t num_properties;

	int (*get_property)(struct accessory *acy,
			    enum accessory_property pacy,
			    union accessory_propval *val);
	int (*put_property) (union accessory_propval *val);


	/* private */
	struct device *dev;

};


extern void accessory_changed(struct accessory *acy);
extern struct accessory* accessory_get_by_name(char* name);

extern int accessory_register(struct device *parent,
				 struct accessory *acy);
extern void accessory_unregister(struct accessory *acy);


#endif /* __ACCESSORY_H__ */

