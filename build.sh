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
    local conftmp="${BUILD_OUTPUT_PATH}/.config_tmp"
    local line=`sed -n '/system/p; /system/q' ${GPT_CONFIG}`
    local arr=(${line//:/ })
    local fs_type=${arr[2]}
    echo "*********************************************************************"
    echo "boot mode = $bootmode"
    echo "*********************************************************************"
    cp ${BUILD_OUTPUT_PATH}/.config $conftmp

    # Create mtdparts string according to gpt.conf
    if [ "$bootmode" = "nor" ];then
        cal_mtdparts
        sed -i "s/CONFIG_MTDPARTS_DEFAULT=.*/CONFIG_MTDPARTS_DEFAULT=\"${mtdparts_str}\"/g" $conftmp
        KCFLAGS=${KCFLAGS}" -DHB_NOR_BOOT"
    elif [[ "$bootmode" = *"nand"* ]];then
        cal_mtdparts
        sed -i "s/CONFIG_MTDPARTS_DEFAULT=.*/CONFIG_MTDPARTS_DEFAULT=\"${mtdparts_str}\"/g" $conftmp
        KCFLAGS=${KCFLAGS}" -DHB_NAND_BOOT"
    fi
    # Here, ROOTFS_TYPE is passed to UBoot for bootargs
    # TODO: Integrate UBIFS in gpt.conf to the build process for partition.sh
    if ! inList "$fs_type" "ext4 squashfs" ;then
        if [ "$bootmode" = "emmc" ]; then
            KCFLAGS=${KCFLAGS}" -DROOTFS_TYPE=ext4"
        elif [ "$bootmode" = "nand" -o "$bootmode" = "nor" ];then
            KCFLAGS=${KCFLAGS}" -DROOTFS_TYPE=ubifs"
        fi
    else
        KCFLAGS=${KCFLAGS}" -DROOTFS_TYPE=${fs_type}"
    fi
    mv $conftmp ${BUILD_OUTPUT_PATH}/.config
}

function build()
{
    local uboot_image="u-boot.bin"
    prefix=$TARGET_UBOOT_DIR
    config=${UBOOT_DEFCONFIG}
    if [ "${BOOT_MODE}" != "emmc" ];then
        config=${config}_${BOOT_MODE}
    fi
    echo "uboot config: $config"

    if [ x"$OPTIMIZE_BOOT_LOG" = x"true" ];then
        KCFLAGS=${KCFLAGS}"-DUBOOT_LOG_OPTIMIZE"
    fi

    # real build
    make ARCH=${ARCH_UBOOT} O=${BUILD_OUTPUT_PATH} $config || {
        echo "make $config failed"
        exit 1
    }

    choose

    make ARCH=${ARCH_UBOOT} O=${BUILD_OUTPUT_PATH} KCFLAGS="${KCFLAGS}" -j${N} || {
        echo "make failed"
        exit 1
    }

    # put binaries to dest directory
    if [ x"$OPTIMIZE_BOOT_LOG" = x"true" ];then
        cp -rf ${BUILD_OUTPUT_PATH}/$uboot_image $prefix/u-boot-optimize.bin
        if [ ! -f $prefix/$uboot_image ];then
            cpfiles "${BUILD_OUTPUT_PATH}/$uboot_image" "$prefix/"
        fi
    else
        cpfiles "${BUILD_OUTPUT_PATH}/$uboot_image" "$prefix/"
    fi

    # Creating standalone UBoot partition image and ota package
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
KCFLAGS=""
mtdparts_str=""

function clean()
{
    make clean
}

# include
. $INCLUDE_FUNCS
# include end

cd $(dirname $0)

if [ "$bootmode" = "nand" ];then
    if [ "$PAGE_SIZE" = "2048" ];then
        export GPT_CONFIG="$SRC_DEVICE_DIR/$TARGET_VENDOR/$TARGET_PROJECT/debug-xj3-nand-gpt.conf"
    elif [ "$PAGE_SIZE" = "4096" -o "$PAGE_SIZE" = "all" ];then
        export GPT_CONFIG="$SRC_DEVICE_DIR/$TARGET_VENDOR/$TARGET_PROJECT/debug-xj3-nand-4096-gpt.conf"
    fi
elif [ "$bootmode" = "nor" ];then
    export GPT_CONFIG="$SRC_DEVICE_DIR/$TARGET_VENDOR/$TARGET_PROJECT/debug-xj3-nor-gpt-ubifs.conf"
else
    arg="emmc"
fi

buildopt $cmd

if [ "$PAGE_SIZE" = "all" -a "$cmd" != "clean" ];then
    # encrypt and sign uboot image
    path="$SRC_BUILD_DIR/tools/key_management_toolkits"
    cd $path
    bash pack_uboot_tool.sh || {
        echo "$SRC_BUILD_DIR/tools/key_management_toolkits/pack_uboot_tool.sh failed"
        exit 1
    }
    cd -
    mv $TARGET_DEPLOY_DIR/uboot/${UBOOT_IMAGE_NAME} $prefix/uboot_4096.img
    export GPT_CONFIG="$SRC_DEVICE_DIR/$TARGET_VENDOR/$TARGET_PROJECT/debug-xj3-nand-gpt.conf"
    buildopt $cmd
fi
