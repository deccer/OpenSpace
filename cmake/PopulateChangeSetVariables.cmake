execute_process(
    COMMAND git rev-list --count HEAD
    OUTPUT_VARIABLE OPENSPACE_REVISION_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if ("${OPENSPACE_REVISION_ID}" STREQUAL "")
    set (OPENSPACE_REVISION_ID 0)
    set (OPENSPACE_SHORT_CHANGE_SET 0)
    set (OPENSPACE_CHANGE_SET 0)
    set (OPENSPACE_CHANGE_SET_DATE "")
else ()
    execute_process(
        COMMAND git rev-parse --short HEAD
        OUTPUT_VARIABLE OPENSPACE_SHORT_CHANGE_SET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND git rev-parse HEAD
        OUTPUT_VARIABLE OPENSPACE_CHANGE_SET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND git log -1 --abbrev=12 --date=format:%Y-%m-%d --pretty=format:%cd
        OUTPUT_VARIABLE OPENSPACE_CHANGE_SET_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif ()
