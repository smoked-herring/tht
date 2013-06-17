isEmpty(TARGET) {
    error("TARGET is not defined")
}

!win32 {
    error("$$TARGET requires Windows platform")
}

INCLUDEPATH += .
DEPENDPATH += .

CONFIG += warn_on
QMAKE_CXXFLAGS_WARN_ON *= -Wextra -Wno-missing-field-initializers

VERSION=$$sprintf("%1.%2.%3", $$NVER1, $$NVER2, $$NVER3)

DEFINES += NVER1=$$NVER1
DEFINES += NVER2=$$NVER2
DEFINES += NVER3=$$NVER3
DEFINES += NVER_STRING=$$sprintf("\"\\\"%1\\\"\"", $$VERSION)

DEFINES += TARGET=$$TARGET

# require at least Windows XP
DEFINES += _WIN32_WINNT=0x0501 WINVER=0x0501

# use fast string concat
DEFINES *= QT_USE_QSTRINGBUILDER

contains(QMAKE_HOST.arch, x86_64) {
    HOST64="-x86_64"
}

# search an executable in PATH
defineReplace(findexe) {
    return ( $$system(for %i in ($$1) do @echo. %~$PATH:i) )
}

# escape command to allow multiple lines in Makefile
defineReplace(mle) {
    return ( $$1$$escape_expand(\\n\\t) )
}

# check for gcc
GCC=$$findexe("gcc.exe")
GCCDIR=$$dirname(GCC)

isEmpty(GCC) {
    error("MinGW is not found in PATH")
}

# copy translations
QMAKE_POST_LINK += $$mle(lupdate -no-obsolete $$_PRO_FILE_)
QMAKE_POST_LINK += $$mle(lrelease $$_PRO_FILE_)
QMAKE_POST_LINK += $$mle(if not exist \"$${OUT_PWD}/$(DESTDIR_TARGET)/../translations\" mkdir \"$${OUT_PWD}/$(DESTDIR_TARGET)/../translations\")

QMFILES=

# copy .qm files
for(ts, TRANSLATIONS) {
    ts=$$replace(ts, \\.ts$, .qm)
    ts=$$replace(ts, /, \\)
    QMFILES=$$QMFILES $$ts
    QMAKE_POST_LINK += $$mle(copy /y \"$${_PRO_FILE_PWD_}\\$$ts\" \"$${OUT_PWD}/$(DESTDIR_TARGET)/../translations\")
}

# check for upx
UPX=$$findexe("upx.exe")

!isEmpty(USEUPX) {
    isEmpty(HOST64):!isEmpty(UPX) {
        message("UPX is found, will pack $$TARGET after linking")

        QMAKE_POST_LINK += $$mle($$UPX -9 \"$${OUT_PWD}/$(DESTDIR_TARGET)\")
        export(QMAKE_POST_LINK)
    } else {
        warning("UPX is not found, will not pack $$TARGET")
    }
}

# check for signtool
SIGNTOOL=$$findexe("signtool.exe")
CERT=$$dirname(_PRO_FILE_)-certs\\cert.pfx

!isEmpty(SIGNTOOL):exists($$CERT) {
    message("Signtool and the certificate are found, will sign the $$TARGET executable")
} else {
    warning("Signtool or the certificate is not found, will not sign the $$TARGET executable")
}

# sign
!isEmpty(SIGNTOOL):exists($$CERT) {
    QMAKE_POST_LINK += $$mle($$SIGNTOOL sign /d \"Trader\'s Home Task\" /du \"$$HTTPROOT\" /f \"$$CERT\" /tr \"http://www.startssl.com/timestamp\" /v \"$${OUT_PWD}/$(DESTDIR_TARGET)\")
}

# check for 7z
ZIP=$$findexe("7z.exe")

# INNO setup
INNO=$$system(echo %ProgramFiles(x86)%)\\Inno Setup 5\\iscc.exe

!exists($$INNO) {
    INNO=$$system(echo %ProgramFiles%)\\Inno Setup 5\\iscc.exe
}