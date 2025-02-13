

all:	clownfish_raw.png

clownfish_raw.png:	clownfish.ans
	../../ansi2gif --png clownfish.ans clownfish_raw.png

####

submit:	clownfish_32x8.zip

clownfish_32x8.zip:	clownfish.png file_id.diz
	mkdir -p lovebyte2025_clownfish_32x8
	cp clownfish.png ./lovebyte2025_clownfish_32x8
	cp clownfish.ans ./lovebyte2025_clownfish_32x8
	cp file_id.diz ./lovebyte2025_clownfish_32x8
	zip -r clownfish_32x8.zip lovebyte2025_clownfish_32x8
