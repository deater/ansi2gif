all:	castle_serenade_raw.png

castle_serenade_raw.png:	castle_serenade.ans
	../../ansi2gif --png castle_serenade.ans castle_serenade_raw.png

####

submit:	castle_serenade_32x8.zip

castle_serenade_32x8.zip:	castle_serenade.png file_id.diz
	mkdir -p lovebyte2025_castle_serenade_32x8
	cp castle_serenade.png ./lovebyte2025_castle_serenade_32x8
	cp castle_serenade.ans ./lovebyte2025_castle_serenade_32x8
	cp file_id.diz ./lovebyte2025_castle_serenade_32x8
	zip -r castle_serenade_32x8.zip lovebyte2025_castle_serenade_32x8
