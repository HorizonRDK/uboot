####################################
# Set you own environment here
####################################

whoami=`whoami`

cross=aarch64-linux-gnu-
arch=arm64
cfg=xj3_soc_defconfig
fs_tmp="rootfs"

####################################
# function define
####################################
mk_cfg()
{
    #echo make menuconfig by using defconfig: $cfg
    make CROSS_COMPILE=$cross ARCH=$arch $cfg
    make CROSS_COMPILE=$cross ARCH=$arch menuconfig
}

mk_all()
{
    #echo make all with $j
    make CROSS_COMPILE=$cross
#   make CROSS_COMPILE=$cross ARCH=$arch dtbs
}

mk_clean()
{
    #echo make clean
    make CROSS_COMPILE=$cross clean
}

helper()
{
    echo
    echo ---------------------------------------------------------------------
    echo "Usage:  "
    echo "  sh mk_kernel.sh [option]"
    echo "    option:"
    echo "    -j: burst build (make -jxx)"
    echo "    -c: make clean command"
    echo "    -m: make menuconfig by specified defconfig (define as cfg above)"
    echo "    -h: helper prompt"
    echo
}

####################################
# main logic from here
####################################

#"uh" > "u:h:" if need args
while getopts "mjch" opt; do
  case $opt in
       m)
	   mk_cfg
	   exit
	   ;;
       j)
           j="-j64"
	   echo burst build !
           ;;
       c)
	   mk_clean
	   exit
	   ;;
       h)
           helper
	   exit
           ;;
      \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

mk_all
