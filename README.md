# u-boot
[![Build images](https://github.com/99degree/u-boot/actions/workflows/build-images.yml/badge.svg)](https://github.com/99degree/u-boot/actions/workflows/build-images.yml)

Das u-boot source tree

This repo is tracking to codelinaro u-boot repo. The development for u-boot under this repo is aimed to add dual boot capability to an XiaoMi redmi note 9 pro int'l version. Currently the working branch is based on latest code
of caleb's public tree with my own compile adjustments. The development under this repo is not aimed to provide full function but barely useble for dual boot only. So interested dev can selectively pick some changes and back merge to caleb's tree if feasible.

Github action manually build with latest 'next' branch for test use, please find it useful.

After changeset[3cf2c0a] there is added support to boot a slightly modified boot.img from LineageOS 22.1 Miatoll build. This is archived by adding some compatible value to bootargs. So fastboot boot cmd is working fine now. The remaining thing is let bootflow cmd works too. Since your developer still not able to get rEFInd boot manager working for boot partition search, so chainloading is not possible but fastboot boot.  

[1] https://github.com/99degree/u-boot/commit/3cf2c0aa42e2e8051fcfc4137c356f6c9cf7659b 
