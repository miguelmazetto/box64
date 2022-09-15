cmake_minimum_required(VERSION 3.19)
project(androidpoly CXX C)
add_library(androidpoly STATIC
	"posix_spawn.cpp"
	"shmem.c"
	#"various.cpp"
	"qsort_r.c"
	"glob.c"
	"__ctype_b_loc.c"
	"__ctype_toupper_loc.c"
	"__ctype_tolower_loc.c"
	"semctl.c"
	"dlinfo.c"
	)
target_link_libraries(androidpoly PUBLIC c++abi log)