#!/bin/bash

function choose()
{
    local tmp="include/configs/.hb_config.h"
    local target="include/configs/hb_config.h"
    local conftmp=".config_tmp"

    echo "*********************************************************************"
    echo "board = $board, boot mode= $bootmode"
    echo "*********************************************************************"
    cp .config $conftmp

    echo "#ifndef __HB_CONFIG_H__" > $tmp
    echo "#define __HB_CONFIG_H__" >> $tmp

    if [ "$bootmode" = "ap" ];then
        echo "#define CONFIG_HB_AP_BOOT" >> $tmp
        echo "/* #define CONFIG_HB_YMODEM_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_NAND_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_NOR_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    elif [ "$bootmode" = "uart" ];then
        echo "/* #define CONFIG_HB_AP_BOOT */" >> $tmp
        echo "#define CONFIG_HB_YMODEM_BOOT" >> $tmp
        echo "/* #define CONFIG_HB_NAND_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_NOR_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=y" >> $conftmp
    elif [ "$bootmode" = "nor" ];then
        echo "/* #define CONFIG_HB_AP_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_YMODEM_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_NAND_BOOT */" >> $tmp
        echo "#define CONFIG_HB_NOR_BOOT" >> $tmp
        echo "/* #define CONFIG_HB_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    elif [ "$bootmode" = "nand" ];then
        echo "/* #define CONFIG_HB_AP_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_YMODEM_BOOT */" >> $tmp
        echo "#define CONFIG_HB_NAND_BOOT" >> $tmp
        echo "/* #define CONFIG_HB_NOR_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    elif [ "$bootmode" = "emmc" ];then
        echo "/* #define CONFIG_HB_AP_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_YMODEM_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_NAND_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_NOR_BOOT */" >> $tmp
        echo "#define CONFIG_HB_MMC_BOOT" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    else
        echo "Unknown BOOT_MODE value: $bootmode"
        exit 1
    fi

    echo "" >> $tmp
    echo "ddr_frequency=$ddr_frequency"
    if [ "$ddr_frequency" = "3200" ];then
        echo "#define CONFIG_X2_LPDDR4_3200 (3200)" >> $tmp
    elif [ "$ddr_frequency" = "2133" ];then
        echo "#define CONFIG_X2_LPDDR4_2133 (2133)" >> $tmp
    elif [ "$ddr_frequency" = "4266" ];then
        echo "#define CONFIG_X2_LPDDR4_4266 (4266)" >> $tmp
    else
        echo "#define CONFIG_X2_LPDDR4_2666 (2666)" >> $tmp
    fi

    echo "" >> $tmp
    if [ "$board" = "som" -o "$board" = "s202" -o "$board" = "cvb" -o "$board" = "dvb" ];then
        echo "#define CONFIG_X2_SOM_BOARD" >> $tmp
        echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
        echo "/* #define CONFIG_X2_QUAD_BOARD */" >> $tmp
        echo "/* #define CONFIG_X2_SK_BOARD */" >> $tmp
    elif [ "$board" = "svb" ];then
        echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
        echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
        echo "/* #define CONFIG_X2_QUAD_BOARD */" >> $tmp
        echo "/* #define CONFIG_X2_SK_BOARD */" >> $tmp
    elif [ "$board" = "mono" ];then
        echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
        echo "#define CONFIG_X2_MONO_BOARD" >> $tmp
        echo "/* #define CONFIG_X2_QUAD_BOARD */" >> $tmp
        echo "/* #define CONFIG_X2_SK_BOARD */" >> $tmp
    elif [ "$board" = "quad" ];then
        echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
        echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
        echo "#define CONFIG_X2_QUAD_BOARD" >> $tmp
        echo "/* #define CONFIG_X2_SK_BOARD */" >> $tmp
    elif [ "$board" = "sk" ];then
        echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
        echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
        echo "/* #define CONFIG_X2_QUAD_BOARD */" >> $tmp
        echo "#define CONFIG_X2_SK_BOARD" >> $tmp
    else
        echo "Unknown BOARD_TYPE value: $board"
        exit 1
    fi

    echo "#endif /* __HB_CONFIG_H__ */" >> $tmp

    mv $tmp $target
    mv $conftmp .config
}

function change_dts_nor_config()
{
    local dts_file="arch/arm/dts/hobot-x3-soc.dts"
    local key_value="qspi {"

    declare -i nline

    getline()
    {
        cat -n $dts_file|grep "${key_value}"|awk '{print $1}'
    }

    getlinenum()
    {
        awk "BEGIN{a=`getline`;b="1";c=(a+b);print c}";
    }

    nline=`getlinenum`

    if [ x"$bootmode" = x"nor" ];then
        sed -i "${nline}s#disabled#okay#g" $dts_file
    else
        sed -i "${nline}s#okay#disabled#g" $dts_file
    fi
}

function build()
{
    prefix=$TARGET_UBOOT_DIR
    config=$UBOOT_DEFCONFIG
    echo "uboot config: $config"

    # real build
    ARCH=$ARCH_KERNEL
    make $config || {
        echo "make $config failed"
        exit 1
    }

#    choose

    make -j${N} || {
        echo "make failed"
        exit 1
    }

    # put binaries to dest directory
    cpfiles "$UBOOT_IMAGE_NAME" "$prefix/"
    # cd spl/
}

function all()
{
    if $all_boot_mode ;then
        bootmode="emmc"
        build
        splname="spl-${board}-$bootmode-${ddr_frequency}.bin"
        mv "u-boot-spl.bin" $splname
        # cpfiles $splname "$prefix/"
        cd ../

        bootmode="nor"
        build
        splname="spl-${board}-$bootmode-${ddr_frequency}.bin"
        mv "u-boot-spl.bin" $splname
        # cpfiles $splname "$prefix/"
        cd ../

        bootmode="nand"
        build
        splname="spl-${board}-$bootmode-${ddr_frequency}.bin"
        mv "u-boot-spl.bin" $splname
        # cpfiles $splname "$prefix/"
        cd ../

        bootmode="uart"
        build
        splname="spl-${board}-$bootmode-${ddr_frequency}.bin"
        mv "u-boot-spl.bin" $splname
        # cpfiles $splname "$prefix/"
        cd ../

        bootmode="ap"
        build
        splname="spl-${board}-$bootmode-${ddr_frequency}.bin"
        mv "u-boot-spl.bin" $splname
        # cpfiles $splname "$prefix/"
        cd ../
    else
        build
        # splname="spl-${board}-$bootmode-${ddr_frequency}.bin"
        # mv "u-boot-spl.bin" $splname
        # cpfiles $splname "$prefix/"
        # cd ../
    fi
}

function all_32()
{
    CROSS_COMPILE=$CROSS_COMPILE_64
    all
}

function set_uboot_config()
{
    if [ "$IMAGE_TYPE" = "nand"  ] | [ "$bootmode" = "nand" ];then
        sed -i 's/# CONFIG_CMD_UBI is not set/CONFIG_CMD_UBI=y/g' $TOPDIR/uboot/configs/hr_x2_defconfig
        sed -i 's/# CONFIG_CMD_UBIFS is not set/CONFIG_CMD_UBIFS=y/g' $TOPDIR/uboot/configs/hr_x2_defconfig
        sed -i 's/# CONFIG_MTD_UBI_FASTMAP is not set/CONFIG_MTD_UBI_FASTMAP=y/g' $TOPDIR/uboot/configs/hr_x2_defconfig
        sed -i 's/# CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT is not set/CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT=1/g' $TOPDIR/uboot/configs/hr_x2_defconfig
    else
        sed -i 's/CONFIG_CMD_UBI=y/# CONFIG_CMD_UBI is not set/g' $TOPDIR/uboot/configs/hr_x2_defconfig
        sed -i 's/CONFIG_CMD_UBIFS=y/# CONFIG_CMD_UBIFS is not set/g' $TOPDIR/uboot/configs/hr_x2_defconfig
        sed -i 's/CONFIG_MTD_UBI_FASTMAP=y/# CONFIG_MTD_UBI_FASTMAP is not set/g' $TOPDIR/uboot/configs/hr_x2_defconfig
        sed -i 's/CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT=1/# CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT is not set/g' $TOPDIR/uboot/configs/hr_x2_defconfig
    fi
    sed -i 's/# CONFIG_CMD_MTDPARTS is not set/CONFIG_CMD_MTDPARTS=y/g' $TOPDIR/uboot/configs/hr_x2_defconfig
}

function usage()
{
    echo "Usage: build.sh [-o uart|emmc|ap|nor|nand|all ] [-b <som|svb|mono|quad|sk> ] [ -d 3200|2666|2133]"
    echo "Options:"
    echo "  -o  boot mode, all or one of uart, emmc, nor, nand, ap"
    echo "  -b  board type "
    echo "  -d  set ddr frequency 3200 or 2666"
    echo "  -h  help info"
    echo "Command:"
    echo "  clean clean all the object files along with the executable"
}

board=$BOARD_TYPE
bootmode=$BOOT_MODE
ddr_frequency=$DDR_FREQ
all_boot_mode=false

while getopts "b:m:o:d:h:" opt
do
    case $opt in
        b)
            export board="$OPTARG"
            ;;
        o)
            arg="$OPTARG"
            if [ "$arg" = "all" ];then
                all_boot_mode=true
                echo "compile all boot mode"
            else
                export bootmode="$OPTARG"
                echo "compile boot mode $bootmode"
            fi
            ;;
        d)
            arg="$OPTARG"
            if [ "$arg" = "2666" ];then
                ddr_frequency="2666"
            elif [ "$arg" = "3200" ];then
                ddr_frequency="3200"
            elif [ "$arg" = "2133" ];then
                ddr_frequency="2133"
            elif [ "$arg" = "4266" ];then
                ddr_frequency="4266"
            else
                usage
                exit 1
            fi
            ;;
        h)
            usage
            exit 0
            ;;
        \?)
            usage
            exit 1
            ;;
    esac
done

shift $[ $OPTIND - 1 ]

cmd=$1

function clean()
{
    make clean
}


# include
. $INCLUDE_FUNCS
# include end

# config dts
change_dts_nor_config

set_uboot_config

cd $(dirname $0)

buildopt $cmd
