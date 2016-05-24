ADD_EXECUTABLE(v9ttd
	utils.c
	v9ttd.c
	vtape.c
	pe.c
	nrz1.c
)

install(TARGETS v9ttd
	RUNTIME DESTINATION bin
)

# vim: tabstop=4
