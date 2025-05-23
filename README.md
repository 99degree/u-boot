# u-boot
[![Build images](https://github.com/99degree/u-boot/actions/workflows/build-images.yml/badge.svg)](https://github.com/99degree/u-boot/actions/workflows/build-images.yml)

Das u-boot source tree

This repo is tracking to latest u-boot repo. The development for u-boot under this repo is aimed to add dual boot capability to an XiaoMi redmi note 9 pro int'l version. Currently the working branch is based on latest code
of u-boot's public tree with my own compile adjustments. The development under this repo is not aimed to provide full function but barely useble for dual boot only. So interested dev can selectively pick some changes and back merge to mainline tree if feasible.

Github action manually build with latest 'next' branch for test use, please find it useful.

After changeset[3cf2c0a] there is added support to boot a slightly modified boot.img from LineageOS 22.1 Miatoll build. This is archived by adding some compatible value to bootargs. So fastboot boot cmd and bootflow cmd[2] are working fine now. 

[1] https://github.com/99degree/u-boot/commit/3cf2c0aa42e2e8051fcfc4137c356f6c9cf7659b 
[2] https://github.com/99degree/u-boot/commit/bddb6870a30bb26405ca49300eec8527e6a1bc2f 
