win_bt_codec.exe: win_bt_codec.cpp
	cl  /std:c++17 win_bt_codec.cpp /link advapi32.lib
