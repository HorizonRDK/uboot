#!/bin/bash

function cal_mtdparts
{
    local total_parts=0
    local part_size=0
    local part_name=""
    local part_start=0
    local part_stop=0
    local part_offset=0
    local part_offset_str="0"
    local arr=""
    local _name=${arr[1]}
    local part=${_name%/*}

    for line in $(cat ${GPT_CONFIG})
    do
        arr=(${line//:/ })

        local needparted=${arr[0]}
        _name=${arr[1]}
        part=${_name%/*}
        local name=${_name##*/}
        local fs=${arr[2]}
        local starts=${arr[3]}
        local start=${starts%?}
        local stops=${arr[4]}
        local stop=${stops%?}

        if [ "${needparted}" = "1" ];then
            if [ "${total_parts}" != "0" ];then
                part_size=$(( (${part_stop} - ${part_start} + 1) * 512 ))
                mtdparts_str="${mtdparts_str}${part_size}@0x${part_offset_str}(${part_name}),"
                part_offset=$(( ${part_offset} + ${part_size} ))
                part_offset_str=$(echo "obase=16; ${part_offset}" | bc)
                part_start=${start}
                part_name=${part}
            else
                part_name=${part}
                mtdparts_str="mtdparts=${mtdids_str##*=}:"
                let total_parts=$(( ${total_parts} + 1 ))
            fi
        fi
        part_stop=${stop}
    done

    mtdparts_str="${mtdparts_str}-@0x${part_offset_str}(${part})"
}

function choose()
{
    local tmp="$cur_dir/include/configs/.hb_config.h"
    local target="$cur_dir/include/configs/hb_config.h"
    local conftmp="$cur_dir/.config_tmp"
    local line=`sed -n '/system/p; /system/q' ${GPT_CONFIG}`
    local arr=(${line//:/ })
    local fs_type=${arr[2]}
    echo "*********************************************************************"
    echo "boot mode = $bootmode"
    echo "*********************************************************************"
    cp $cur_dir/.config $conftmp

    echo "#ifndef __HB_CONFIG_H__" > $tmp
    echo "#define __HB_CONFIG_H__" >> $tmp

    if [ "$bootmode" = "nor" ]|| [[ "$FLASH_ENABLE" = "nor" ]];then
        mtdids_str="spi-nor1=hr_nor.0"
        if [[ "$bootmode" = "nor" ]];then
            echo "/* #define CONFIG_HB_NAND_BOOT */" >> $tmp
            echo "#define CONFIG_HB_NOR_BOOT" >> $tmp
            echo "/* #define CONFIG_HB_MMC_BOOT */" >> $tmp
            sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
            echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
            sed -i 's/CONFIG_ENV_IS_IN_MMC=y/# CONFIG_ENV_IS_IN_MMC is not set/g' $conftmp
            sed -i 's/# CONFIG_ENV_IS_IN_SPI_FLASH is not set/CONFIG_ENV_IS_IN_SPI_FLASH=y/g' $conftmp
        fi
        # Create mtdparts string according to gpt.conf
        cal_mtdparts
        sed -i "s/CONFIG_MTDIDS_DEFAULT=.*/CONFIG_MTDIDS_DEFAULT=\"${mtdids_str}\"/g"  $conftmp
        sed -i "s/CONFIG_MTDPARTS_DEFAULT=.*/CONFIG_MTDPARTS_DEFAULT=\"${mtdparts_str}\"/g" $conftmp
    elif [[ "$bootmode" = *"nand"* ]];then
        mtdids_str="spi-nand0=hr_nand.0"
        if [[ "$bootmode" = "nand" ]];then
            echo "#define CONFIG_HB_NAND_BOOT" >> $tmp
            echo "/* #define CONFIG_HB_NOR_BOOT */" >> $tmp
            echo "/* #define CONFIG_HB_MMC_BOOT */" >> $tmp
            sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
            echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
            sed -i 's/CONFIG_ENV_IS_IN_MMC=y/# CONFIG_ENV_IS_IN_MMC is not set/g' $conftmp
            sed -i 's/# CONFIG_ENV_IS_IN_UBI is not set/CONFIG_ENV_IS_IN_UBI=y/g' $conftmp
        fi
        # Create mtdparts string according to gpt.conf
        cal_mtdparts
        sed -i "s/CONFIG_MTDIDS_DEFAULT=.*/CONFIG_MTDIDS_DEFAULT=\"${mtdids_str}\"/g"  $conftmp
        sed -i "s/CONFIG_MTDPARTS_DEFAULT=.*/CONFIG_MTDPARTS_DEFAULT=\"${mtdparts_str}\"/g" $conftmp
    elif [ "$bootmode" = "emmc" ];then
        echo "/* #define CONFIG_HB_NAND_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_NOR_BOOT */" >> $tmp
        echo "#define CONFIG_HB_MMC_BOOT" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    else
        echo "Unknown BOOT_MODE value: $bootmode"
        exit 1
    fi
    if ! inList "$fs_type" "ext4 squashfs" ;then
        echo "#define ROOTFS_TYPE \"ext4\"" >> $tmp
    else
        echo "#define ROOTFS_TYPE \"$fs_type\"" >> $tmp
    fi
    echo "#endif /* __HB_CONFIG_H__ */" >> $tmp

    mv $tmp $target
    mv $conftmp .config
}

function change_dts_flash_config()
{
    local dts_file="$cur_dir/arch/arm/dts/hobot-x3-soc.dts"
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

    if [ x"$bootmode" = x"nor" ] || [ x"$bootmode" = x"nand" ] \
       || [ ! -z "$FLASH_ENABLE" ];then
        sed -i "${nline}s#disabled#okay#g" $dts_file
    else
        sed -i "${nline}s#okay#disabled#g" $dts_file
    fi
}

function build()
{
    # config dts
    # change_dts_flash_config

    local uboot_image="u-boot.bin"
    prefix=$TARGET_UBOOT_DIR
    config=$UBOOT_DEFCONFIG
    echo "uboot config: $config"

    if [ x"$OPTIMIZE_BOOT_LOG" = x"true" ];then
        OPTIMIZE_LOG="-DUBOOT_LOG_OPTIMIZE"
    fi

    # real build
    ARCH=$ARCH_KERNEL
    make $config || {
        echo "make $config failed"
        exit 1
    }

    choose

    make KCFLAGS="$OPTIMIZE_LOG" -j${N} || {
        echo "make failed"
        exit 1
    }

    # put binaries to dest directory
    if [ x"$OPTIMIZE_BOOT_LOG" = x"true" ];then
        cp -rf $uboot_image $prefix/u-boot-optimize.bin
        if [ ! -f $prefix/$uboot_image ];then
            cpfiles "$uboot_image" "$prefix/"
        fi
    else
        cpfiles "$uboot_image" "$prefix/"
    fi

    if [ "$pack_img" = "true" ];then
        local path_otatool="$SRC_BUILD_DIR/ota_tools/uboot_package_maker"
        if [ -d "$prefix/" ];then
            # encrypt and sign uboot image
            path="$SRC_BUILD_DIR/tools/key_management_toolkits"
            cd $path
            bash pack_uboot_tool.sh || {
                echo "$SRC_BUILD_DIR/tools/key_management_toolkits/pack_uboot_tool.sh failed"
                exit 1
            }
            cd -
        fi
        local line=`sed -n '/UBOOT_IMAGE_NAME/p; /UBOOT_IMAGE_NAME/q' ${GPT_CONFIG}`
        local arr=(${line//:/ })
        local starts=${arr[3]}
        local stops=${arr[4]}
        local stop=${stops%?}
        local start=${starts%?}
        local uboot_size=$(( ${stop} - ${start} + 1 ))
        rm -rf $TARGET_DEPLOY_DIR/uboot.img
        runcmd "dd if=/dev/zero of=$TARGET_DEPLOY_DIR/uboot.img bs=512 count=${uboot_size} conv=notrunc,sync status=none"
        runcmd "dd if=$TARGET_DEPLOY_DIR/uboot/$UBOOT_IMAGE_NAME of=$TARGET_DEPLOY_DIR/uboot.img conv=notrunc,sync status=none"
        runcmd "dd if=$TARGET_DEPLOY_DIR/uboot/$UBOOT_IMAGE_NAME of=$TARGET_DEPLOY_DIR/uboot.img bs=512 seek=2048 conv=notrunc,sync status=none"
        cd $path_otatool
            bash build_uboot_update_package.sh emmc
        cd -
        cpfiles "$path_otatool/uboot.zip" "$TARGET_DEPLOY_DIR/ota"
    fi
}

function all()
{
    build
}

function all_32()
{
    CROSS_COMPILE=$CROSS_COMPILE_64
    all
}

function usage()
{
    echo "Usage: build.sh [-o emmc|nor|nand|nand_4096 ] [-u] [-p] [-l]"    echo "Options:"
    echo "Options:"
    echo "  -o  boot mode, all or one of uart, emmc, nor, nand, ap"
    echo "  -p  create uboot.img"
    echo "  -l  gets the minimum boot log image"
    echo "  -h  help info"
    echo "Command:"
    echo "  clean clean all the object files along with the executable"
}

bootmode=$BOOT_MODE
pack_img="false"
while getopts "upo:lh:" opt
do
    case $opt in
        o)
            arg="$OPTARG"
            if [ "$arg" = "nand" ];then
                export PAGE_SIZE="2048"
            elif [ "$arg" = "nand_4096" ];then
                export PAGE_SIZE="4096"
            elif [ "$arg" = "nor" ];then
                arg="nor"
            else
                arg="emmc"
            fi
            export bootmode="$arg"
            echo "compile boot mode $bootmode"
            ;;
        p)
            pack_img="true"
            ;;
        l)
            OPTIMIZE_BOOT_LOG="true"
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
mtdids_str=""
mtdparts_str=""

function clean()
{
    make clean
}

# include
. $INCLUDE_FUNCS
# include end
cur_dir=${OUT_BUILD_DIR}/uboot

cd $(dirname $0)

if [ "$bootmode" = "nand" ];then
    if [ "$PAGE_SIZE" = "2048" ];then
        export GPT_CONFIG="$SRC_DEVICE_DIR/$TARGET_VENDOR/$TARGET_PROJECT/debug-x3-nand-gpt.conf"
    elif [ "$PAGE_SIZE" = "4096" -o "$PAGE_SIZE" = "all" ];then
        export GPT_CONFIG="$SRC_DEVICE_DIR/$TARGET_VENDOR/$TARGET_PROJECT/debug-x3-nand-4096-gpt.conf"
    fi
elif [ "$bootmode" = "nor" ];then
    export GPT_CONFIG="$SRC_DEVICE_DIR/$TARGET_VENDOR/$TARGET_PROJECT/debug-x3-nor-gpt-ubifs.conf"
else
    arg="emmc"
fi

buildopt $cmd

if [ "$PAGE_SIZE" = "all" ];then
    # encrypt and sign uboot image
    path="$SRC_BUILD_DIR/tools/key_management_toolkits"
    cd $path
    bash pack_uboot_tool.sh || {
        echo "$SRC_BUILD_DIR/tools/key_management_toolkits/pack_uboot_tool.sh failed"
        exit 1
    }
    cd -
    mv $TARGET_DEPLOY_DIR/uboot/${UBOOT_IMAGE_NAME} $prefix/uboot_4096.img
    export GPT_CONFIG="$SRC_DEVICE_DIR/$TARGET_VENDOR/$TARGET_PROJECT/debug-x3-nand-gpt.conf"
    buildopt $cmd
fi
