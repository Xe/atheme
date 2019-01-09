AC_DEFUN([ATHEME_DECIDE_DIGEST_FRONTEND], [

	DIGEST_FRONTEND_VAL="ATHEME_DIGEST_FRONTEND_INTERNAL"
	DIGEST_FRONTEND="Internal"
	DIGEST_API_CFLAGS=""
	DIGEST_API_LIBS=""

	AS_IF([test "${DIGEST_FRONTEND}${LIBMBEDCRYPTO}" = "InternalYes"], [
		DIGEST_FRONTEND_VAL="ATHEME_DIGEST_FRONTEND_MBEDTLS"
		DIGEST_FRONTEND="ARM mbedTLS"
		DIGEST_API_CFLAGS=""
		DIGEST_API_LIBS="${LIBMBEDCRYPTO_LIBS}"
	])

	AS_IF([test "${DIGEST_FRONTEND}${LIBNETTLE}" = "InternalYes"], [
		DIGEST_FRONTEND_VAL="ATHEME_DIGEST_FRONTEND_NETTLE"
		DIGEST_FRONTEND="Nettle"
		DIGEST_API_CFLAGS="${LIBNETTLE_CFLAGS}"
		DIGEST_API_LIBS="${LIBNETTLE_LIBS}"
	])

	AS_IF([test "${DIGEST_FRONTEND}${LIBCRYPTO}" = "InternalYes"], [
		DIGEST_FRONTEND_VAL="ATHEME_DIGEST_FRONTEND_OPENSSL"
		DIGEST_FRONTEND="${LIBCRYPTO_NAME}"
		DIGEST_API_CFLAGS="${LIBCRYPTO_CFLAGS}"
		DIGEST_API_LIBS="${LIBCRYPTO_LIBS}"
	])

	AC_DEFINE_UNQUOTED([ATHEME_DIGEST_FRONTEND], [${DIGEST_FRONTEND_VAL}], [Front-end for digest interface])
	AC_SUBST([DIGEST_API_CFLAGS])
	AC_SUBST([DIGEST_API_LIBS])
])
