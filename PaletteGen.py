

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


def IsTwoHex(token):
	bIsTwoHex = False
	tempToken = token
	if tempToken.startswith("0x") or tempToken.startswith("0X"):
		tempToken = token[2:]
		if (tempToken != ""):
			if len(tempToken) == 2:
				if (all(c in "0123456789abcdefABCDEF" for c in tempToken)):
					bIsTwoHex = True
	return bIsTwoHex

def main():

	tableFileH = open("palettes.txt", 'r')
	fileH = open("PaletteGen.txt", 'w')
	
	totalChecksumList = []

	checksumList = []
	BGVec = []
	OBJ0 = []
	OBJ1 = []
	lines = tableFileH.readlines()
	for line in lines:
		# if two hex format add to checksum list
		tokens = line.split()
		for tokenIDX in range(len(tokens)):
			token = tokens[tokenIDX]
			if IsTwoHex(token):
				if (token not in totalChecksumList):
					checksumList.append(token)
					totalChecksumList.append(token)
				continue
			elif token.upper() == "BG":
				BGVec.append(tokens[tokenIDX + 1])
				BGVec.append(tokens[tokenIDX + 2])
				BGVec.append(tokens[tokenIDX + 3])
				BGVec.append(tokens[tokenIDX + 4])
				break
			elif token.upper() == "OBJ0":
				OBJ0.append(tokens[tokenIDX + 1])
				OBJ0.append(tokens[tokenIDX + 2])
				OBJ0.append(tokens[tokenIDX + 3])
				OBJ0.append(tokens[tokenIDX + 4])
				break
			elif token.upper() == "OBJ1":
				OBJ1.append(tokens[tokenIDX + 1])
				OBJ1.append(tokens[tokenIDX + 2])
				OBJ1.append(tokens[tokenIDX + 3])
				OBJ1.append(tokens[tokenIDX + 4])
				
				if (len(checksumList) > 0):
					for checksum in checksumList:
						fileH.write("case " + checksum + ":\n")
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
				
				checksumList = []
				BGVec = []
				OBJ0 = []
				OBJ1 = []
				break

main()
