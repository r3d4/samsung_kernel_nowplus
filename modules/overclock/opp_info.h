/**
 * struct omap_opp - OMAP OPP description structure
 * @enabled:	true/false - marking this OPP as enabled/disabled
 * @rate:	Frequency in hertz
 * @u_volt:	Nominal voltage in microvolts corresponding to this OPP
 * @opp_id:	opp identifier (deprecated)
 *
 * This structure stores the OPP information for a given domain.
 */
struct omap_opp {
	struct list_head node;

	bool enabled;
	unsigned long rate;
	unsigned long u_volt;
	u8 opp_id;

	struct device_opp *dev_opp;  /* containing device_opp struct */
};

struct device_opp {
	struct list_head node;

	struct omap_hwmod *oh;
	struct device *dev;

	struct list_head opp_list;
	u32 opp_count;
	u32 enabled_opp_count;

	int (*set_rate)(struct device *dev, unsigned long rate);
	unsigned long (*get_rate) (struct device *dev);
};

/* Voltage processor register offsets */
struct vp_reg_offs {
	u8 vpconfig;
	u8 vstepmin;
	u8 vstepmax;
	u8 vlimitto;
	u8 vstatus;
	u8 voltage;
};

/* Voltage Processor bit field values, shifts and masks */
struct vp_reg_val {
	/* VPx_VPCONFIG */
	u32 vpconfig_erroroffset;
	u16 vpconfig_errorgain;
	u32 vpconfig_errorgain_mask;
	u8 vpconfig_errorgain_shift;
	u32 vpconfig_initvoltage_mask;
	u8 vpconfig_initvoltage_shift;
	u32 vpconfig_timeouten;
	u32 vpconfig_initvdd;
	u32 vpconfig_forceupdate;
	u32 vpconfig_vpenable;
	/* VPx_VSTEPMIN */
	u8 vstepmin_stepmin;
	u16 vstepmin_smpswaittimemin;
	u8 vstepmin_stepmin_shift;
	u8 vstepmin_smpswaittimemin_shift;
	/* VPx_VSTEPMAX */
	u8 vstepmax_stepmax;
	u16 vstepmax_smpswaittimemax;
	u8 vstepmax_stepmax_shift;
	u8 vstepmax_smpswaittimemax_shift;
	/* VPx_VLIMITTO */
	u16 vlimitto_vddmin;
	u16 vlimitto_vddmax;
	u16 vlimitto_timeout;
	u16 vlimitto_vddmin_shift;
	u16 vlimitto_vddmax_shift;
	u16 vlimitto_timeout_shift;
	/* PRM_IRQSTATUS*/
	u32 tranxdone_status;
};

/**
 * omap_vdd_dep_volt - Table containing the parent vdd voltage and the
 *			dependent vdd voltage corresponding to it.
 *
 * @main_vdd_volt	: The main vdd voltage
 * @dep_vdd_volt	: The voltage at which the dependent vdd should be
 *			  when the main vdd is at <main_vdd_volt> voltage
 */
struct omap_vdd_dep_volt {
	u32 main_vdd_volt;
	u32 dep_vdd_volt;
};

/**
 *  ABB Register offsets and masks
 *
 * @prm_abb_ldo_setup_idx : PRM_LDO_ABB_SETUP Register specific to MPU/IVA
 * @prm_abb_ldo_ctrl_idx  : PRM_LDO_ABB_CTRL Register specific to MPU/IVA
 * @prm_irqstatus_mpu	  : PRM_IRQSTATUS_MPU_A9/PRM_IRQSTATUS_MPU_A9_2
 * @abb_done_st_shift	  : ABB_DONE_ST shift
 * @abb_done_st_mask	  : ABB_DONE_ST_MASK bit mask
 *
 */
struct abb_reg_val {
	u16 prm_abb_ldo_setup_idx;
	u16 prm_abb_ldo_ctrl_idx;
	u16 prm_irqstatus_mpu;
	u32 abb_done_st_shift;
	u32 abb_done_st_mask;
};

/**
 * omap_vdd_dep_info - Dependent vdd info
 *
 * @name		: Dependent vdd name
 * @voltdm		: Dependent vdd pointer
 * @dep_table		: Table containing the dependent vdd voltage
 *			  corresponding to every main vdd voltage.
 * @cur_dep_volt	: The voltage to which dependent vdd should be put
 *			  to for the current main vdd voltage.
 */
struct omap_vdd_dep_info{
	char *name;
	struct voltagedomain *voltdm;
	struct omap_vdd_dep_volt *dep_table;
	unsigned long cur_dep_volt;
};

/**
 * omap_vdd_user_list	- The per vdd user list
 *
 * @dev		: The device asking for the vdd to be set at a particular
 *		  voltage
 * @node	: The list head entry
 * @volt	: The voltage requested by the device <dev>
 */
struct omap_vdd_user_list {
	struct device *dev;
	struct plist_node node;
	u32 volt;
};

/**
 * omap_vdd_info - Per Voltage Domain info
 *
 * @volt_data		: voltage table having the distinct voltages supported
 *			  by the domain and other associated per voltage data.
 * @vp_offs		: structure containing the offsets for various
 *			  vp registers
 * @vp_reg		: the register values, shifts, masks for various
 *			  vp registers
 * @volt_clk		: the clock associated with the vdd.
 * @opp_dev		: the 'struct device' associated with this vdd.
 * @user_lock		: the lock to be used by the plist user_list
 * @user_list		: the list head maintaining the various users
 *			  of this vdd with the voltage requested by each user.
 * @volt_data_count	: Number of distinct voltages supported by this vdd.
 * @nominal_volt	: Nominal voltaged for this vdd.
 * cmdval_reg		: Voltage controller cmdval register.
 * @vdd_sr_reg		: The smartreflex register associated with this VDD.
 */
struct omap_vdd_info{
	struct omap_volt_data *volt_data;
	struct vp_reg_offs vp_offs;
	struct vp_reg_val vp_reg;
	struct clk *volt_clk;
	struct device *opp_dev;
	struct voltagedomain voltdm;
	struct abb_reg_val omap_abb_reg_val;
	struct omap_vdd_dep_info *dep_vdd_info;
	spinlock_t user_lock;
	struct plist_head user_list;
	struct mutex scaling_mutex;
	struct srcu_notifier_head volt_change_notify_list;
	int volt_data_count;
	int nr_dep_vdd;
	struct device **dev_list;
	int dev_count;
	unsigned long nominal_volt;
	unsigned long curr_volt;
	u8 cmdval_reg;
	u8 vdd_sr_reg;
	struct omap_volt_pmic_info *pmic;
	struct device vdd_device;
};
