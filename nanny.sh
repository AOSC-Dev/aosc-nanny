#!/bin/bash -e

export TEXTDOMAIN=aosc_launcher
. gettext.sh
USE_GUI=1

cpu_baseline_error() {
    if ! grep -P "^flags\s+: .+?\s($1)" /proc/cpuinfo; then
        local features="$1"
        MESSAGE_TEXT="$(eval_gettext 'Your processor does not support the "${features}" feature, which is required by ${PKGNAME}. This application will therefore not function correctly on your device and will now terminate.\n')"
        show
        exit 10
    fi
}

telemetry_warning() {
    GENERIC_LEGAL_DOC_NAME="$(eval_gettext 'Licensing Terms')"
    LEGAL_DOC_NAME="${LEGAL_DOC_NAME:-$GENERIC_LEGAL_DOC_NAME}"
    MESSAGE_TEXT="$(eval_gettext '${PKGDES} may collect your usage data on an opt-out basis, per the <a href=\"${EULA_URL}\">${LEGAL_DOC_NAME}</a>.\n\nThis default setting does not comply with our guidelines on telemetry in packaged software, per section 5 of the <a href=\"https://wiki.aosc.io/developer/packaging/package-styling-manual/#package-features\">AOSC OS Packaging Styling Manual</a>. ')"
    if [[ -n "${ALT_SOFTWARE}" ]]; then
        MESSAGE_TEXT+="$(eval_gettext 'We offer a Telemetry-free alternative, ${ALT_SOFTWARE} (package: ${ALT_PACKAGE}).\n\n')"
    else
        MESSAGE_TEXT+="\n\n"
    fi
    MESSAGE_TEXT+="$(eval_gettext 'Would you like to proceed with launching ${PKGNAME}?\n')"
    PROMPT_TEXT="$(eval_gettext 'By selecting "Yes," you agree to the licensing terms referenced above and consent launching an application which violates our packaging guidelines.')"
    show
}

show_gui() {
    if [[ -n "${3}" ]]; then
        zenity \
            --question \
            --window-icon="/usr/share/icons/breeze/status/48/dialog-warning.svg" \
            --icon-name="dialog-warning" \
            --width=640 \
            --title "$1" \
            --text "$2\n$3"
        return $?
    else
        zenity \
            --warning \
            --width=640 \
            --title "$1" \
            --text "$2"
    fi
}

show_cli() {
    if ! w3m -version > /dev/null; then
        printf "$1\n$2\n"
    else
        printf "<html><h1>$1</h1><p>$2</p><p>$3</p></html>" | w3m -T text/html -dump
    fi
    if [[ -n "$3" ]]; then
        read -p "$(eval_gettext 'Proceed? [y/N]') " -r RESPONSE
        if [ "$RESPONSE" != "y" ] && [ "$RESPONSE" != "Y" ]; then
            echo ''
            eval_gettext 'You have chosen not to proceed. Exiting...'
            echo ''
            return 20
        fi
    fi
}

show() {
    if [ -n "${DISPLAY}" ] && [ -n "${USE_GUI}" ]; then
        show_gui "$(eval_gettext 'Warning')" "${MESSAGE_TEXT}" "${PROMPT_TEXT}"
    else
        show_cli "$(eval_gettext 'Warning')" "${MESSAGE_TEXT}" "${PROMPT_TEXT}"
    fi
}

usage() {
    echo -e "
Usage:	$0 [-n PKGNAME] [-a ALT_SOFTWARE] [-k ALT_PACKAGE] [-d PKGDES]
	[-l EULA_URL] [-f CPU_FEATURE] [-c]

	-n	PKGNAME: Name of the offending package.
	-a	ALT_SOFTWARE: Name of alternative software (if applicable).
	-k 	ALT_PACKAGE: Name of alternative package (if applicable).
		You pass -a with -k.
	-d	PKGDES: Description of the offending package (usually the
		"pretty name" for said application).
	-l	EULA_URL: URL to the licensing terms.
	-f	CPU_FEATURE: Required processor feature.

	-c	Launch in command line.
"
}

telemetry_warning_check() {
    mkdir -p "$HOME"/.config/
    touch "$HOME"/.config/nanny.db
    if grep -F "$PKGNAME" "$HOME"/.config/nanny.db > /dev/null; then
        exit 0
    fi
    if telemetry_warning; then
        echo "$PKGNAME" >> "$HOME"/.config/nanny.db
        exit 0
    fi
    exit 20
}

if [ $# -eq 0 ]; then
    usage
    exit 1
fi

while getopts ":n:a:k:l:d:f:c" options; do
    case "${options}" in
        n)
            export PKGNAME="${OPTARG}"
            ;;
        a)
            ALT_SOFTWARE="${OPTARG}"
            ;;
        k)
            ALT_PACKAGE="${OPTARG}"
            ;;
        d)
            PKGDES="${OPTARG}"
            ;;
        l)
            EULA_URL="${OPTARG}"
            ;;
        f)
            CPU_FEATURE="${OPTARG}"
            ;;
        c)
            unset USE_GUI
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

if [[ -n "${CPU_FEATURE}" ]]; then
    cpu_baseline_error "${CPU_FEATURE}"
fi

if [[ -n "${PKGDES}" ]]; then
    telemetry_warning_check
fi
