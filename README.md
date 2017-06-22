# R41N3OW
R41N3OW is an application used for Rainbow Tables generation and hash cracking.
It's made for educational purposes, as a university project.

## Supported features
* Rainbow Tables generation with variable:
    * table size
    * chain lenght
    * hash function
    * reduction function
    * duplicate chain retries
    * threads used
    * starting passwords
* Cracking given plaintext, using previously created table
* Managing binary & text files
* Hashing given plaintext using:
    * BLAKE2b
    * SHA-1
    * SHA-256
* Test mode for testing created table with random passwords.

## Dependencies
To be able to use and/or compile the code OpenSSL library is needed!
After installing OpenSSL, the link to the main OpenSSL directory has to be made in deps project directory - instructions inside.

#### Windows
For Ms Windows following installer is suggested for the ease of use:
https://slproweb.com/products/Win32OpenSSL.html (please do not rely on the Lite version)

Application was tested with 'Win64 OpenSSL v1.1.0f' from the above link.
#### Linux
As for linux based systems - application is using OpenSSL and STD libraries, both of which are available on unix operating systems. It is worth noting, that even though application should work under unix os, it was never tested in such way.
