s/\\033/\\[/g
/'.'/s/'/"/g
#/\'.*[\'/s/\'/\"/g
/'.*'/s/'/\"/g
/U_/y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/
s/	u_/	/g
s/	NULL/	""/g
s/TTSCP_PORT/8778/g
