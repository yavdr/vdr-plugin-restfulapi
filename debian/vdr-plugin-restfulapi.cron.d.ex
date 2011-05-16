#
# Regular cron jobs for the vdr-plugin-restfulapi package
#
0 4	* * *	root	[ -x /usr/bin/vdr-plugin-restfulapi_maintenance ] && /usr/bin/vdr-plugin-restfulapi_maintenance
