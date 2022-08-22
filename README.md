Nanny
=====

An application advisory system for AOSC OS.

Nanny currently supports the following advisory modes:

- Opt-out telemetry warning.
- Processor feature errors, preempts SIGILL errors.

Nanny could run either as a Zenity dialog or as a command line output.

Usage
-----

Below are the availabe command line switches for Nanny.

```
Usage:  nanny [-n PKGNAME] [-a ALT_SOFTWARE] [-k ALT_PACKAGE] [-d PKGDES]
	[-l EULA_URL] [-f CPU_FEATURE] [-c]

	-n      PKGNAME: Name of the offending package.
	-a      ALT_SOFTWARE: Name of alternative software (if applicable).
	-k      ALT_PACKAGE: Name of alternative package (if applicable).
	You pass -a with -k.
	-d      PKGDES: Description of the offending package (usually the
	"pretty name" for said application).
	-l      EULA_URL: URL to the licensing terms.
	-f      CPU_FEATURE: Required processor feature.

	-c      Launch in command line.
```

Nanny records and stores user response at `~/.config/nanny.db`.

Dependencies
------------

- Breeze icon theme.
- w3m.
- Zenity.
