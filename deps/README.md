R41N30W - Deps
==============

Dependencies are gathered here. You can copy them to the appropriate folders, but we recommend using symlinks/junction links to point at your SDKs install location.

Right now we require:

* OpenSSL - symlink called "openssl-(platform)", pointing to your Windows OpenSSL installation directory (usually C:\OpenSSL-Win32 for or C:\OpenSSL-Win64):
  * openssl-x86 for OpenSSL-Win32 build
  * openssl-x64 for OpenSSL-Win64 build

Make the links through console. On Windows:

```
mklink /J openssl-x64 <path_to_Win64_installation>
```
