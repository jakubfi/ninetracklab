ADD_EXECUTABLE(v9ttd
	vtape.c
	v9ttd.c
	utils.c
)

install(TARGETS v9ttd
	RUNTIME DESTINATION bin
)

# vim: tabstop=4
