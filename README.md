# national_embedded_competition_2021

# smart_lock

#### Introduction

全国大学生嵌入式竞赛海思赛道-智能门锁赛题-相关代码

#### Installation

1. Pegasus‘Code Patch Validation

- step1: Clone this repository to your local environment.
- step2: Re-unpack a HiHope_WiFi_loT_Hi3861SPC025's SDK file.
- step3: Move all documents under index national_embedded_competition_2021_smart_lock/Pegasus to the index HiHope_WiFi-loT_Hi3861SPC025, choose subusitute all.
- step4: Open HUAWEI LiteOS Studio, then open project HiHope_WiFi-loT_Hi3861S
  PC025, compile it first and burn the result into Pegasus development board.
- step5: Verify functions

2. Taurus’code Patch Validation

- step1: Clone this repository to your local environment.
- step2: Follow instructions offered by Hispark QQ chat group, under index code, re-construct a environment  of hiopenais_V1.2
- step3: Copy all files in national_embedded_competition_2021_smart_lock/Taurus/ to code/hiopenais/patch
- step4: Execute following commands

```
patch -p1 < patch/0001.hiopenais_build.patch
patch -p2 < patch/0002.hiopenais_src.patch
patch -p3 < patch/0003.hiopenais_sdk_taurus.patch
patch -p4 < patch/0004.hiopenais_sdk.patch
patch -p5 < patch/0005.hiopenais_sdk_output.patch
patch -p6 < patch/0006.hiopenais_sdk_rootfs_glibc.patch
patch -p7 < patch/0007.hiopenais_install_sh.patch
```

- step5: Enter code/hiopenais/build and execute following commands, recompile hiopenais

```
make rebuild && make plugs_rebuild && make boards_rebuild
```

- step6: Move all files in code/hiopenais/output to code/hiopenais/sdk/hi3516dv300/rootfs_glibc/pub/hiopenais
- step7: Enter code/hiopenais/sdk/hi3516dv300/rootfs_glibc/pub/hiopenais, execute following commands, rebuild file system

```
./build.sh ext4
```

- step8: Put your wk file in code/hiopenais/sdk/hi3516dv300/rootfs_glibc/pub/hiopenais/plugs
- step9: Open Hitool and burn patitial.xml into Taurus board
- step10: Verify functions

#### Precautions for use

1.  This project need a electronic lock, 2 power supplies that can supply 12 volts and some connecting cables.
2.  The model is trained to recognize our 3 members of this project, so you are not expected unlock the lock.
