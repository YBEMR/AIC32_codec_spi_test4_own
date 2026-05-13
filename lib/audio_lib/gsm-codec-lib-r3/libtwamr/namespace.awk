# This awk script generates namespace.h from namespace.list

BEGIN {
	print "/* Auto-generated; do not edit! */"
	print ""
	print "#ifndef	namespace_h"
	print "#define	namespace_h"
	print ""
}

{
	for (i = 1; i <= NF; i++) {
		if ($i ~ /^#/)
			break;
		printf "#define	%s	amr__%s\n", $i, $i;
	}
}

END {
	print ""
	print "#endif"
}
