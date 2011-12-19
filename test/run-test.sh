#!/bin/bash -eu

TMP_DIR=$(mktemp -d /tmp/polipo-test.XXXXXX)
POLIPO_DIR=$PWD
POLIPO_TEST_DIR=${POLIPO_DIR}/test
LOG_LEVEL=0xFF
TEST_DEBUG=true

cd "$TMP_DIR"
"$TEST_DEBUG" && echo "Sandboxed in ${TMP_DIR}" 1>&2

mkdir "${TMP_DIR}/cache_root"
mkdir "${TMP_DIR}/responses"

function fail {
	echo "FAIL: $1" 1>&2
	echo "Polipo log:"
	cat polipo_log.txt
	exit 1
}

# Start polipo and set POLIPO_PID, POLIPO_PORT and http_proxy
function start_polipo {
	export $(${POLIPO_TEST_DIR}/sd-launch -l polipo_log.txt ${POLIPO_DIR}/polipo -c ${POLIPO_TEST_DIR}/polipo.conf diskCacheRoot=${TMP_DIR}/cache_root logLevel=${LOG_LEVEL} | sed s/LAUNCHED_/POLIPO_/g)
	"$TEST_DEBUG" && echo "Polipo started pid=${POLIPO_PID} address=http://localhost:${POLIPO_PORT}/" 1>&2
	export http_proxy=http://localhost:${POLIPO_PORT}/
}

function stop_polipo {
	kill "${POLIPO_PID}"
}

function run_test_case {
	FILENAME_ROOT=$1
	EXPECTED_REQUESTS_SERVED=$2

	PAYLOAD=${FILENAME_ROOT}.payload
	HEADER=${FILENAME_ROOT}.header
	RESPONSE=${TMP_DIR}/responses/$(basename ${FILENAME_ROOT})
	SAVE_LOCATION=${TMP_DIR}/out/$(basename ${FILENAME_ROOT})

	${TEST_DEBUG} && echo "Running test ${FILENAME_ROOT}"

	mkdir -p $(dirname ${SAVE_LOCATION})

	cat "$HEADER" "$PAYLOAD" > "$RESPONSE"

	# Start http server and set HTTP_SERVER_PID and HTTP_SERVER_PORT
	# The test web server will serve any URL so it doesn't matter which one we
	# choose
	export $(${POLIPO_TEST_DIR}/sd-launch -l http_log.txt ${POLIPO_TEST_DIR}/http-test-webserver "${RESPONSE}" | sed s/LAUNCHED_/HTTP_SERVER_/g)

	start_polipo
	for n in {1..10}
	do
		curl -o "${SAVE_LOCATION}" "http://localhost:${HTTP_SERVER_PORT}/" 2>/dev/null
		cmp "${PAYLOAD}" "${SAVE_LOCATION}" || fail "Files don't match"
		rm "${SAVE_LOCATION}"
	done
	stop_polipo

	for n in {1..10}
	do
		(
			start_polipo
			curl -o "${SAVE_LOCATION}" "http://localhost:${HTTP_SERVER_PORT}/" 2>/dev/null
			stop_polipo
		)
		cmp "${PAYLOAD}" "${SAVE_LOCATION}" || fail "Files don't match"
		rm "${SAVE_LOCATION}"
	done

	kill "${HTTP_SERVER_PID}"

	export $(<http_log.txt)
	rm http_log.txt
	[ $REQUESTS_SERVED == $EXPECTED_REQUESTS_SERVED ] || fail "TOO MANY REQUESTS: ${REQUESTS_SERVED}"
}

for HEADER in ${POLIPO_TEST_DIR}/expect_cache/*.header
do
	run_test_case ${HEADER%%.header} 1
done

for HEADER in ${POLIPO_TEST_DIR}/expect_uncached/*.header
do
	run_test_case ${HEADER%%.header} 20
done

rm -R "${TMP_DIR}"
echo SUCCESS

