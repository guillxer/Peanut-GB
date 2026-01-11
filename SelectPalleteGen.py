



def RGBTo555(RBGString):
	r = RBGString[2:4]
	g = RBGString[4:6]
	b = RBGString[6:8]

	rFloat = int(r, 16) / 255.0
	gFloat = int(g, 16) / 255.0
	bFloat = int(b, 16) / 255.0

	palette555 = 0
	palette555 = int(min(31, int(31 * rFloat))) << 10
	palette555 |= int(min(31, int(31 * gFloat))) << 5
	palette555 |= int(min(31, int(31 * bFloat)))

	return hex(palette555)


def main():

	tableFileH = open("selectpalettes.txt", 'r')
	fileH = open("SelectPaletteGen.txt", 'w')

	tokenIDX = 0
	selectIndex = 0
	BGVec = []
	OBJ0 = []
	OBJ1 = []
	lines = tableFileH.readlines()
	lineIndex = 0
	for line in lines:
		tokens = line.split()
		if lineIndex % 3 == 0:
			BGVec.append(tokens[tokenIDX + 0])
			BGVec.append(tokens[tokenIDX + 1])
			BGVec.append(tokens[tokenIDX + 2])
			BGVec.append(tokens[tokenIDX + 3])
		elif lineIndex % 3 == 1:
			OBJ0.append(tokens[tokenIDX + 0])
			OBJ0.append(tokens[tokenIDX + 1])
			OBJ0.append(tokens[tokenIDX + 2])
			OBJ0.append(tokens[tokenIDX + 3])
		elif lineIndex % 3 == 2:
			OBJ1.append(tokens[tokenIDX + 0])
			OBJ1.append(tokens[tokenIDX + 1])
			OBJ1.append(tokens[tokenIDX + 2])
			OBJ1.append(tokens[tokenIDX + 3])
		
			fileH.write("case " + str(selectIndex) + ":")
			fileH.write("{\n")
			fileH.write("	const uint16_t palette[3][4] =\n")
			fileH.write("	{\n")
			fileH.write("		{ ")
			for i in range(4): #OBJ0
				fileH.write(RGBTo555(OBJ0[i]) + ", ")
			fileH.write("},\n")
			fileH.write("		{ ")
			for i in range(4): #OBJ1
				fileH.write(RGBTo555(OBJ1[i]) + ", ")
			fileH.write("},\n")
			fileH.write("		{ ")
			for i in range(4): #BG
				fileH.write(RGBTo555(BGVec[i]) + ", ")
			fileH.write("},\n")
			fileH.write("	};\n")
			
			fileH.write("   for (int x = 0; x < 3; ++x) {\n")
			fileH.write("	    for (int y = 0; y < 4; ++y) {\n")
			fileH.write("		    priv->selected_palette[x][y] = palette[x][y];\n")
			fileH.write("		}\n")
			fileH.write("	}\n")
			fileH.write("	break;\n")
			fileH.write("}\n")
			
			selectIndex += 1
			BGVec = []
			OBJ0 = []
			OBJ1 = []
			
		lineIndex += 1

main()