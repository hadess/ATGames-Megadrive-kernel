# ATGames-Megadrive-kernel

ATGames provided a kernel snapshot for their Android-based Mega Drive
emulator console. This README describes the specific purpose of this
repository. It is not a development repository, but the snapshot of
the kernel sources for a specific product.

## Rebasing information

- Hardware vendor is Rockchip, so base our rebase on their kernel:
  https://github.com/rockchip-linux/kernel/
- Makefile of our snapshot says:
    ```
    VERSION = 3 
    PATCHLEVEL = 10
    ```
  So checkout the release-3.10 branch
- Makefile of our snapshot says:
    ```
    SUBLEVEL = 49
    ```
  So the kernel is probably based off a commit between the 3.10.49 and 3.10.50
  or between `d02dae430d1f41f2c1dc8592f68569f90b86f6d9` and 
  `92488f4c9f687cc0e274be561f7b168743f59f20`
  that's 57 commits to check in all
- `git reset --hard 92488f4c9f687cc0e274be561f7b168743f59f20`
  `git rebase -i d02dae430d1f41f2c1dc8592f68569f90b86f6d9`
- Replace all "pick" with "edit"
  ```
- And run this to create diffs for each differing files:
  `while test -d .git/rebase-merge/ ; do diff -uprN --exclude=.git/ ./ ~/at-games-kernel > ../`git rev-parse HEAD`.patch ; git rebase --continue ; done`
- We end up with a folder that includes 57 patches, look for the smallest one 
- The 3.10.49 base commit (`d02dae430d1f41f2c1dc8592f68569f90b86f6d9`) is the smallest one
