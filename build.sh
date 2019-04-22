#!/bin/bash

function choose()
{
    local tmp="include/configs/.x2_config.h"
    local target="include/configs/x2_config.h"
    local conftmp=".config_tmp"

    echo "*********************************************************************"
    echo "board = $board, boot mode= $bootmode"
    echo "*********************************************************************"
    cp .config $conftmp

    echo "#ifndef __X2_CONFIG_H__" > $tmp
    echo "#define __X2_CONFIG_H__" >> $tmp

    if [ "$bootmode" = "ap" ];then
        echo "#define CONFIG_X2_AP_BOOT" >> $tmp
        echo "/* #define CONFIG_X2_YMODEM_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_NOR_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    elif [ "$bootmode" = "uart" ];then
        echo "/* #define CONFIG_X2_AP_BOOT */" >> $tmp
        echo "#define CONFIG_X2_YMODEM_BOOT" >> $tmp
        echo "/* #define CONFIG_X2_NOR_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=y" >> $conftmp
    elif [ "$bootmode" = "nor" ];then
        echo "/* #define CONFIG_X2_AP_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_YMODEM_BOOT */" >> $tmp
        echo "#define CONFIG_X2_NOR_BOOT" >> $tmp
        echo "/* #define CONFIG_X2_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    elif [ "$bootmode" = "emmc" ];then
        echo "/* #define CONFIG_X2_AP_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_YMODEM_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_NOR_BOOT */" >> $tmp
        echo "#define CONFIG_X2_MMC_BOOT" >> $tmp
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
    else
        echo "#define CONFIG_X2_LPDDR4_2666 (2666)" >> $tmp
    fi

    echo "" >> $tmp
    if [ "$board" = "x2som" ];then
        echo "#define CONFIG_X2_SOM_BOARD" >> $tmp
        echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
    elif [ "$board" = "x2svb" ];then
        echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
        echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
    elif [ "$board" = "x2mono" ];then
        echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
        echo "#define CONFIG_X2_MONO_BOARD" >> $tmp
    else
        echo "Unknown BOARD_TYPE value: $board"
        exit 1
    fi

    echo "#endif /* __X2_CONFIG_H__ */" >> $tmp

    mv $tmp $target
    mv $conftmp .config
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

    choose

    make -j${N} || {
        echo "make failed"
        exit 1
    }

    # put binaries to dest directory
    cpfiles "$UBOOT_IMAGE_NAME" "$prefix/"
    cd spl/
}

function all()
{
    if $all_boot_mode ;then
        bootmode="emmc"
        build
        splname="spl-${board_type}-$bootmode-${ddr_frequency}.bin"
        mv "u-boot-spl.bin" $splname
        cpfiles $splname "$prefix/"
        cd ../

        bootmode="nor"
        build
        splname="spl-${board_type}-$bootmode-${ddr_frequency}.bin"
        mv "u-boot-spl.bin" $splname
        cpfiles $splname "$prefix/"
        cd ../

        bootmode="uart"
        build
        splname="spl-${board_type}-$bootmode-${ddr_frequency}.bin"
        mv "u-boot-spl.bin" $splname
        cpfiles $splname "$prefix/"
        cd ../

        bootmode="ap"
        build
        splname="spl-${board_type}-$bootmode-${ddr_frequency}.bin"
        mv "u-boot-spl.bin" $splname
        cpfiles $splname "$prefix/"
        cd ../
    else
        build
        splname="spl-${board_type}-$bootmode-${ddr_frequency}.bin"
        mv "u-boot-spl.bin" $splname
        cpfiles $splname "$prefix/"
        cd ../
    fi
}


function usage()
{
    echo "Usage: build.sh [-o uart|emmc|ap|nor|all ] [-b <som|svb|mono> ] [ -d 3200|2666]"
    echo "Options:"
    echo "  -o  boot mode, all or one of uart, emmc, nor, ap"
    echo "  -b  board type "
    echo "  -d  set ddr frequency 3200 or 2666"
    echo "  -h  help info"
    echo "Command:"
    echo "  clean clean all the object files along with the executable"
}

board=$BOARD_TYPE
bootmode=$BOOT_MODE
ddr_frequency="2666"
all_boot_mode=false
if [ "$board" = "x2som" ];then
    board_type="som"
elif [ "$board" == "x2svb" ];then
    board_type="svb"
elif [ "$board" = "x2mono" ];then
    board_type="mono"
fi

while getopts "b:m:o:d:h:" opt
do
    case $opt in
        b)
            export board_type="$OPTARG"
            board="x2$board_type"
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

function clean()
{
    make clean
}


# include
. $INCLUDE_FUNCS
# include end

cd $(dirname $0)

buildopt $cmd
