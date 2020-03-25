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

    # echo "" >> $tmp
    # echo "ddr_frequency=$ddr_frequency"
    # if [ "$ddr_frequency" = "3200" ];then
    #     echo "#define CONFIG_X2_LPDDR4_3200 (3200)" >> $tmp
    # elif [ "$ddr_frequency" = "2133" ];then
    #     echo "#define CONFIG_X2_LPDDR4_2133 (2133)" >> $tmp
    # elif [ "$ddr_frequency" = "4266" ];then
    #     echo "#define CONFIG_X2_LPDDR4_4266 (4266)" >> $tmp
    # else
    #     echo "#define CONFIG_X2_LPDDR4_2666 (2666)" >> $tmp
    # fi

    # echo "" >> $tmp
    # if [ "$board" = "som" -o "$board" = "s202" -o "$board" = "cvb" -o "$board" = "dvb" ];then
    #     echo "#define CONFIG_X2_SOM_BOARD" >> $tmp
    #     echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
    #     echo "/* #define CONFIG_X2_QUAD_BOARD */" >> $tmp
    #     echo "/* #define CONFIG_X2_SK_BOARD */" >> $tmp
    # elif [ "$board" = "svb" ];then
    #     echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
    #     echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
    #     echo "/* #define CONFIG_X2_QUAD_BOARD */" >> $tmp
    #     echo "/* #define CONFIG_X2_SK_BOARD */" >> $tmp
    # elif [ "$board" = "mono" ];then
    #     echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
    #     echo "#define CONFIG_X2_MONO_BOARD" >> $tmp
    #     echo "/* #define CONFIG_X2_QUAD_BOARD */" >> $tmp
    #     echo "/* #define CONFIG_X2_SK_BOARD */" >> $tmp
    # elif [ "$board" = "quad" ];then
    #     echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
    #     echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
    #     echo "#define CONFIG_X2_QUAD_BOARD" >> $tmp
    #     echo "/* #define CONFIG_X2_SK_BOARD */" >> $tmp
    # elif [ "$board" = "sk" ];then
    #     echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
    #     echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
    #     echo "/* #define CONFIG_X2_QUAD_BOARD */" >> $tmp
    #     echo "#define CONFIG_X2_SK_BOARD" >> $tmp
    # else
    #     echo "Unknown BOARD_TYPE value: $board"
    #     exit 1
    # fi

    echo "#endif /* __HB_CONFIG_H__ */" >> $tmp

    mv $tmp $target
    mv $conftmp .config
}

function change_dts_flash_config()
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

    if [ x"$bootmode" = x"nor" ] || [ x"$bootmode" = x"nand" ];then
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

    choose

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
    sed -i '/CONFIG_MTDIDS_DEFAULT/d' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
    sed -i '/CONFIG_MTDPARTS_DEFAULT/d' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
    if [[ "$IMAGE_TYPE" = "nand" ]] || [[ "$bootmode" = "nand" ]] || [[ "$ifubi" = "true" ]];then
        sed -i 's/# CONFIG_MTD_UBI_FASTMAP is not set/CONFIG_MTD_UBI_FASTMAP=y/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/# CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT is not set/CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT=1/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/CONFIG_ENV_IS_IN_MMC=y/# CONFIG_ENV_IS_IN_MMC is not set/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/# CONFIG_ENV_IS_IN_UBI is not set/CONFIG_ENV_IS_IN_UBI=y/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_CMD_MTDPARTS/a CONFIG_MTDIDS_DEFAULT="spi-nand0=hr_nand"'  $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_MTDIDS_DEFAULT/a CONFIG_MTDPARTS_DEFAULT="mtdparts=hr_nand:6815744@0x0(bootloader),20971520@0x680000(sys),62914560@0x1A80000(rootfs),-@0x5680000(userdata)"' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
    elif [[ "$bootmode" = "nor" ]];then
        sed -i 's/# CONFIG_SPI_FLASH_MTD is not set/CONFIG_SPI_FLASH_MTD=y/g'
        sed -i '/CONFIG_CMD_MTDPARTS/a CONFIG_MTDIDS_DEFAULT="spi-nor1=hr_nor"'  $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_MTDIDS_DEFAULT/a CONFIG_MTDPARTS_DEFAULT="mtdparts=hr_nor:393216@0x20000(sbl),524288@0x80000(bl31),1048576@0x100000(uboot),131072@0x200000(bpu),131072@0x220000(vbmeta),10485760@0x240000(boot),10485760@0xC40000(recovery),34603008@0x1640000(system),-@0x3740000(app)"' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
    else
        sed -i 's/CONFIG_MTD_UBI_FASTMAP=y/# CONFIG_MTD_UBI_FASTMAP is not set/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT=1/# CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT is not set/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/# CONFIG_ENV_IS_IN_MMC is not set/CONFIG_ENV_IS_IN_MMC=y/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/CONFIG_ENV_IS_IN_UBI=y/# CONFIG_ENV_IS_IN_UBI is not set/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_CMD_MTDPARTS/a CONFIG_MTDIDS_DEFAULT=""'  $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_MTDIDS_DEFAULT/a CONFIG_MTDPARTS_DEFAULT=""' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
    fi
    sed -i 's/# CONFIG_CMD_MTDPARTS is not set/CONFIG_CMD_MTDPARTS=y/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
}

function usage()
{
    echo "Usage: build.sh [-o uart|emmc|ap|nor|nand|all ] [-b <som|svb|mono|quad|sk> ] [ -d 3200|2666|2133] [-u]"    echo "Options:"
    echo "Options:"
    echo "  -o  boot mode, all or one of uart, emmc, nor, nand, ap"
    echo "  -b  board type "
    echo "  -d  set ddr frequency 3200 or 2666"
    echo "  -u  add ubi and ubifs support in uboot"
    echo "  -h  help info"
    echo "Command:"
    echo "  clean clean all the object files along with the executable"
}

board=$BOARD_TYPE
bootmode=$BOOT_MODE
ddr_frequency=$DDR_FREQ
all_boot_mode=false
ifubi=false

while getopts "b:m:o:d:uh:" opt
do
    case $opt in
        b)
            export board="$OPTARG"
            ;;
        u)
            ifubi=true
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
change_dts_flash_config

set_uboot_config

cd $(dirname $0)

buildopt $cmd
