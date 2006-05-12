typedef struct {
	char const * name;
	struct output_conf_s conf;
} mode_info_t;

static const mode_info_t mode_info[];

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static int param_set_output_mode_t(const char *val, struct kernel_param *kp)
{
	if (val) {
		int i;
		for (i=0; i < MODE_MAX; i++)
			if (strcmp(val, mode_info[i].name) == 0) {
				*(output_mode_t *)kp->arg = i;
				return 0;
			}
	}
	printk(KERN_ERR "%s: output_mode parameter expected\n",
	       kp->name);
	return -EINVAL;
}

static int param_get_output_mode_t(char *buffer, struct kernel_param *kp)
{
	return sprintf(buffer, "%s", mode_info[*(output_mode_t *)kp->arg].name);
}
#endif
