<div align="center">
  <a href="https://ice.io">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://ice.io/download/ion_logo_dark_background.svg">
      <img alt="ION logo" src="https://ice.io/download/ion_logo_light_background.svg">
    </picture>
  </a>
  <h3>Reference implementation of ION Node and tools</h3>
  <hr/>
</div>

## 

<p align="center">
  <a href="https://ionresear.ch">
    <img src="https://img.shields.io/badge/ION%20Research-0098EA?style=flat&logo=discourse&label=Forum&labelColor=gray" alt="Ton Research">
  </a>
  <a href="https://t.me/ioncoin">
    <img src="https://img.shields.io/badge/ION%20Community-0098EA?logo=telegram&logoColor=white&style=flat" alt="Telegram Community Group">
  </a>
  <a href="https://t.me/ionblockchain">
    <img src="https://img.shields.io/badge/ION%20Foundation-0098EA?logo=telegram&logoColor=white&style=flat" alt="Telegram Foundation Group">
  </a>
  <a href="https://t.me/iondev_eng">
    <img src="https://img.shields.io/badge/chat-IONDev-0098EA?logo=telegram&logoColor=white&style=flat" alt="Telegram Community Chat">
  </a>
</p>

<p align="center">
  <a href="https://twitter.com/ice_blockchain">
    <img src="https://img.shields.io/twitter/follow/ice_blockchain" alt="Twitter Group">
  </a>
  <a href="https://answers.ion.org">
    <img src="https://img.shields.io/badge/-ION%20Overflow-FE7A16?style=flat&logo=stack-overflow&logoColor=white" alt="ION Overflow Group">
  </a>
  <a href="https://stackoverflow.com/questions/tagged/ion">
    <img src="https://img.shields.io/badge/-Stack%20Overflow-FE7A16?style=flat&logo=stack-overflow&logoColor=white" alt="Stack Overflow Group">
  </a>
</p>



Main ION monorepo, which includes the code of the node/validator, lite-client, tonlib, FunC compiler, etc.

## Ice Open Network

__Ice Open Network (ION)__ is a fast, secure, scalable blockchain focused on handling _millions of transactions per second_ (TPS) with the goal of reaching hundreds of millions of blockchain users.
- To learn more about different aspects of ION blockchain and its underlying ecosystem check [documentation](https://docs.ice.io)
- To run node, validator or lite-server check [Participate section](https://docs.ice.io/ion-blockchain/validator)
- To develop decentralised apps check [Tutorials](https://docs.ion.org/v3/guidelines/smart-contracts/guidelines), [FunC docs](https://docs.ice.io/develop/func/overview) and [DApp tutorials](https://docs.ion.org/v3/guidelines/dapps/overview)
- To work on ION check [wallets](https://ion.app/wallets), [explorers](https://ion.app/explorers), [DEXes](https://ion.app/dex) and [utilities](https://ion.app/utilities)
- To interact with ION check [APIs](https://docs.ion.org/v3/guidelines/dapps/apis-sdks/overview)

## Updates flow

* **master branch** - mainnet is running on this stable branch.

    Only emergency updates, urgent updates, or updates that do not affect the main codebase (GitHub workflows / docker images / documentation) are committed directly to this branch.

* **testnet branch** - testnet is running on this branch. The branch contains a set of new updates. After testing, the testnet branch is merged into the master branch and then a new set of updates is added to testnet branch.

* **backlog** - other branches that are candidates to getting into the testnet branch in the next iteration.

Usually, the response to your pull request will indicate which section it falls into.


## "Soft" Pull Request rules

* Thou shall not merge your own PRs, at least one person should review the PR and merge it (4-eyes rule)
* Thou shall make sure that workflows are cleanly completed for your PR before considering merge

## Build ION blockchain

### Ubuntu 20.4, 22.04, 24.04 (x86-64, aarch64)
Install additional system libraries
```bash
  sudo apt-get update
  sudo apt-get install -y build-essential git cmake ninja-build zlib1g-dev libsecp256k1-dev libmicrohttpd-dev libsodium-dev
          
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  sudo ./llvm.sh 16 all
```
Compile ION binaries
```bash
  cp assembly/native/build-ubuntu-shared.sh .
  chmod +x build-ubuntu-shared.sh
  ./build-ubuntu-shared.sh  
```

### MacOS 11, 12 (x86-64, aarch64)
```bash
  cp assembly/native/build-macos-shared.sh .
  chmod +x build-macos-shared.sh
  ./build-macos-shared.sh
```

### Windows 10, 11, Server (x86-64)
You need to install `MS Visual Studio 2022` first.
Go to https://www.visualstudio.com/downloads/ and download `MS Visual Studio 2022 Community`.

Launch installer and select `Desktop development with C++`. 
After installation, also make sure that `cmake` is globally available by adding
`C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin` to the system `PATH` (adjust the path per your needs).

Open an elevated (Run as Administrator) `x86-64 Native Tools Command Prompt for VS 2022`, go to the root folder and execute: 
```bash
  copy assembly\native\build-windows.bat .
  build-windows.bat
```

### Building ION to WebAssembly
Install additional system libraries on Ubuntu
```bash
  sudo apt-get update
  sudo apt-get install -y build-essential git cmake ninja-build zlib1g-dev libsecp256k1-dev libmicrohttpd-dev libsodium-dev
          
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  sudo ./llvm.sh 16 all
```
Compile ION binaries with emscripten
```bash
  cd assembly/wasm
  chmod +x fift-func-wasm-build-ubuntu.sh
  ./fift-func-wasm-build-ubuntu.sh
```

### Building ION tonlib library for Android (arm64-v8a, armeabi-v7a, x86, x86-64)
Install additional system libraries on Ubuntu
```bash
  sudo apt-get update
  sudo apt-get install -y build-essential git cmake ninja-build automake libtool texinfo autoconf libgflags-dev \
  zlib1g-dev libssl-dev libreadline-dev libmicrohttpd-dev pkg-config libgsl-dev python3 python3-dev \
  libtool autoconf libsodium-dev libsecp256k1-dev
```
Compile ION tonlib library
```bash
  cp assembly/android/build-android-tonlib.sh .
  chmod +x build-android-tonlib.sh
  ./build-android-tonlib.sh
```

### ION portable binaries

Linux portable binaries are wrapped into AppImages, at the same time MacOS portable binaries are statically linked executables.
Linux and MacOS binaries are available for both x86-64 and arm64 architectures. 

## Running tests

Tests are executed by running `ctest` in the build directory. See `doc/Tests.md` for more information.
