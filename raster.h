
#if defined(MICROPY_ENABLE_DYNRUNTIME)
#define RP2 1
#else
#define RP2 0
#endif

//#include <math.h>

#if RP2
#else
//#include <string>
//#include <cmath>
//#include <fstream>
#define PI 3.14159265359f
#endif

#define screenmultiple 1

#define width 128 * screenmultiple
#define height 128 * screenmultiple
#undef near
#undef far
#define near 0.01f
#define far 10.0f
// TODO enable
#define farclip false

#define bUseBariFill false

#define BG_PALLET 2

#if !RP2
void SaveTileData(char* tileDataPath);
void LoadTileData(char* tileDataPath);
#endif

#if RP2
// TODO byte only memcpy, add long copies
// TODO reverse copies
void* memcpy(void* dst, const void* src, size_t len) {
	size_t i;
	unsigned char* d = (unsigned char*)dst;
	const unsigned char* s = (unsigned char*)src;

	for (i = 0; i < len; ++i) {
		d[i] = s[i];
	}
	return dst;
}

void* memset(void* dst, int val, size_t len) {
	size_t i;
	unsigned char* d = (unsigned char*)dst;

	for (i = 0; i < len; ++i) {
		d[i] = val;
	}
	return dst;
}
#endif

typedef struct {
	bool AlphaClipEnable;
	unsigned short AlphaClipColor;
} BlendState;

typedef struct {
	bool UseMeshColor;
	bool UseVertexColor;
	unsigned short MeshColor;
	bool DrawGrid;
} ShaderState;

typedef struct {
	BlendState blendstate;
	ShaderState shaderstate;
	unsigned short* color;
	float* depth;
	float* cameraposition;
	float* viewmatrix;
	float* projectionmatrix;
	int numverts;
	float* vertices;
	int tileX;
	int tileY;
	struct gb_s* gb;
	unsigned int tileIndex;
	bool sampleBG;
	unsigned char attributes;
} DrawMeshArgs;

unsigned short FloatTo565(float* Color) {
	unsigned int Red = (unsigned int)(Color[0] * 31.0f) & 0x1F;
	unsigned int Green = (unsigned int)(Color[1] * 63.0f) & 0x3F;
	unsigned int Blue = (unsigned int)(Color[2] * 31.0f) & 0x1F;
	return (Red << 11) | (Green << 5) | Blue;
}

BlendState GetBlendState(float* StateArray) {
	BlendState State;
	State.AlphaClipEnable = StateArray[0] != 0;
	State.AlphaClipColor = FloatTo565(&StateArray[1]);
	return State;
}

ShaderState GetShaderState(float* StateArray) {
	ShaderState State;
	State.UseMeshColor = StateArray[0] != 0;
	State.UseVertexColor = StateArray[1] != 0;
	State.DrawGrid = StateArray[2] != 0;
	State.MeshColor = FloatTo565(&StateArray[3]);
	return State;
}

void rastermesh(DrawMeshArgs* meshargs);
void rastermeshoffset(DrawMeshArgs* meshargs, float* positionOffset);

#if RP2

static mp_obj_t DrawTrianglesTexScratch(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args) {
	int argindex = 0;
	mp_buffer_info_t blendstatebuffer;
	mp_get_buffer_raise(args[argindex++], &blendstatebuffer, MP_BUFFER_RW);
	mp_buffer_info_t shaderstatebuffer;
	mp_get_buffer_raise(args[argindex++], &shaderstatebuffer, MP_BUFFER_RW);
	mp_buffer_info_t colorbuffer;
	mp_get_buffer_raise(args[argindex++], &colorbuffer, MP_BUFFER_RW);
	mp_buffer_info_t depthbuffer;
	mp_get_buffer_raise(args[argindex++], &depthbuffer, MP_BUFFER_RW);
	mp_buffer_info_t camerapositionbuffer;
	mp_get_buffer_raise(args[argindex++], &camerapositionbuffer, MP_BUFFER_RW);
	mp_buffer_info_t viewmatrixbuffer;
	mp_get_buffer_raise(args[argindex++], &viewmatrixbuffer, MP_BUFFER_RW);
	mp_buffer_info_t projectionmatrixbuffer;
	mp_get_buffer_raise(args[argindex++], &projectionmatrixbuffer, MP_BUFFER_RW);
	int numverts = mp_obj_get_int(args[argindex++]);
	int vertexaddress = mp_obj_get_int(args[argindex++]);

	BlendState blendstate = GetBlendState((float*)blendstatebuffer.buf);
	ShaderState shaderstate = GetShaderState((float*)shaderstatebuffer.buf);
	unsigned short* color = (unsigned short*)colorbuffer.buf;
	float* depth = (float*)depthbuffer.buf;
	float* cameraposition = (float*)camerapositionbuffer.buf;
	float* viewmatrix = (float*)viewmatrixbuffer.buf;
	float* projectionmatrix = (float*)projectionmatrixbuffer.buf;
	float* vertices = (float*)(void*)vertexaddress;

	DrawMeshArgs meshargs;
	meshargs.blendstate = blendstate;
	meshargs.shaderstate = shaderstate;
	meshargs.color = color;
	meshargs.depth = depth;
	meshargs.cameraposition = cameraposition;
	meshargs.viewmatrix = viewmatrix;
	meshargs.projectionmatrix = projectionmatrix;
	meshargs.numverts = numverts;
	meshargs.vertices = vertices;

	rastermesh(&meshargs);

	return mp_const_none;
}

static mp_obj_t DrawTrianglesTex(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args) {

	int argindex = 0;
	mp_buffer_info_t blendstatebuffer;
	mp_get_buffer_raise(args[argindex++], &blendstatebuffer, MP_BUFFER_RW);
	mp_buffer_info_t shaderstatebuffer;
	mp_get_buffer_raise(args[argindex++], &shaderstatebuffer, MP_BUFFER_RW);
	mp_buffer_info_t colorbuffer;
	mp_get_buffer_raise(args[argindex++], &colorbuffer, MP_BUFFER_RW);
	mp_buffer_info_t depthbuffer;
	mp_get_buffer_raise(args[argindex++], &depthbuffer, MP_BUFFER_RW);
	mp_buffer_info_t camerapositionbuffer;
	mp_get_buffer_raise(args[argindex++], &camerapositionbuffer, MP_BUFFER_RW);
	mp_buffer_info_t viewmatrixbuffer;
	mp_get_buffer_raise(args[argindex++], &viewmatrixbuffer, MP_BUFFER_RW);
	mp_buffer_info_t projectionmatrixbuffer;
	mp_get_buffer_raise(args[argindex++], &projectionmatrixbuffer, MP_BUFFER_RW);
	int numverts = mp_obj_get_int(args[argindex++]);
	mp_buffer_info_t vertexbuffer;
	mp_get_buffer_raise(args[argindex++], &vertexbuffer, MP_BUFFER_RW);

	BlendState blendstate = GetBlendState((float*)blendstatebuffer.buf);
	ShaderState shaderstate = GetShaderState((float*)shaderstatebuffer.buf);
	unsigned short* color = (unsigned short*)colorbuffer.buf;
	float* depth = (float*)depthbuffer.buf;
	float* cameraposition = (float*)camerapositionbuffer.buf;
	float* viewmatrix = (float*)viewmatrixbuffer.buf;
	float* projectionmatrix = (float*)projectionmatrixbuffer.buf;
	float* vertices = (float*)vertexbuffer.buf;

	DrawMeshArgs meshargs;
	meshargs.blendstate = blendstate;
	meshargs.shaderstate = shaderstate;
	meshargs.color = color;
	meshargs.depth = depth;
	meshargs.cameraposition = cameraposition;
	meshargs.viewmatrix = viewmatrix;
	meshargs.projectionmatrix = projectionmatrix;
	meshargs.numverts = numverts;
	meshargs.vertices = vertices;

	rastermesh(&meshargs);

	return mp_const_none;
}

static mp_obj_t ClearRenderTarget(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args) {

	int argindex = 0;
	mp_buffer_info_t ColorTargetBuffer;
	mp_get_buffer_raise(args[argindex++], &ColorTargetBuffer, MP_BUFFER_RW);
	mp_buffer_info_t ColorValueBuffer;
	mp_get_buffer_raise(args[argindex++], &ColorValueBuffer, MP_BUFFER_RW);

	unsigned short* ColorTarget = (unsigned short*)ColorTargetBuffer.buf;
	float* ColorValue = (float*)ColorValueBuffer.buf;
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			unsigned int Red = (unsigned int)(ColorValue[0] * 31.0f) & 0x1F;
			unsigned int Green = (unsigned int)(ColorValue[1] * 63.0f) & 0x3F;
			unsigned int Blue = (unsigned int)(ColorValue[2] * 31.0f) & 0x1F;
			ColorTarget[y * height + x] = (Red << 11) | (Green << 5) | Blue;
		}
	}
	return mp_const_none;
}

static mp_obj_t ClearDepthBuffer(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args) {

	int argindex = 0;
	mp_buffer_info_t DepthTargetBuffer;
	mp_get_buffer_raise(args[argindex++], &DepthTargetBuffer, MP_BUFFER_RW);
	mp_buffer_info_t DepthValueBuffer;
	mp_get_buffer_raise(args[argindex++], &DepthValueBuffer, MP_BUFFER_RW);
	float DepthValue = ((float*)DepthValueBuffer.buf)[0];

	float* DepthBuffer = (float*)DepthTargetBuffer.buf;
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			DepthBuffer[x * height + y] = DepthValue;
		}
	}
	return mp_const_none;
}

mp_obj_t mpy_init(mp_obj_fun_bc_t* self, size_t n_args, size_t n_kw, mp_obj_t* args) {
	MP_DYNRUNTIME_INIT_ENTRY
	mp_store_global(
		MP_QSTR_DrawTrianglesTex,
		MP_DYNRUNTIME_MAKE_FUNCTION(DrawTrianglesTex));
	mp_store_global(
		MP_QSTR_DrawTrianglesTexScratch,
		MP_DYNRUNTIME_MAKE_FUNCTION(DrawTrianglesTexScratch));
	mp_store_global(
		MP_QSTR_ClearRenderTarget,
		MP_DYNRUNTIME_MAKE_FUNCTION(ClearRenderTarget));
	mp_store_global(
		MP_QSTR_ClearDepthBuffer,
		MP_DYNRUNTIME_MAKE_FUNCTION(ClearDepthBuffer));
	MP_DYNRUNTIME_INIT_EXIT
}
#endif

float invsqrtf(float n) {
	n = 1.0f / n;
	long i;
	float x = 0.0f;
	float y = 0.0f;

	x = n * 0.5f;
	y = n;
	memcpy(&i, &y, sizeof(float));
	i = 0x5f3759df - (i >> 1);
	memcpy(&y, &i, sizeof(float));
	y = y * (1.5f - (x * y * y));

	return y;
}

#if RP2
float sqrtf(float a) {
	return 1.0f / invsqrtf(a);
}
#endif

typedef struct {
	float x;
	float y;
	float z;
} vec3;
vec3 vecadd(vec3 a, vec3 b) {
	vec3 out;
	out.x = a.x + b.x;
	out.y = a.y + b.y;
	out.z = a.z + b.z;
	return out;
}
vec3 vecsub(vec3 a, vec3 b) {
	vec3 out;
	out.x = a.x - b.x;
	out.y = a.y - b.y;
	out.z = a.z - b.z;
	return out;
}
vec3 vecmul(vec3 a, vec3 b) {
	vec3 out;
	out.x = a.x * b.x;
	out.y = a.y * b.y;
	out.z = a.z * b.z;
	return out;
}
vec3 vecmuls(vec3 a, float b) {
	vec3 out;
	out.x = a.x * b;
	out.y = a.y * b;
	out.z = a.z * b;
	return out;
}
vec3 vecneg(vec3 a) {
	vec3 out;
	out = vecmuls(a, -1.0f);
	return out;
}
vec3 vecdiv(vec3 a, vec3 b) {
	vec3 out;
	out.x = a.x / b.x;
	out.y = a.y / b.y;
	out.z = a.z / b.z;
	return out;
}
vec3 vecdivs(vec3 a, float b) {
	vec3 out;
	out.x = a.x / b;
	out.y = a.y / b;
	out.z = a.z / b;
	return out;
}
float dot(vec3 a, vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
float mag(vec3 a) {
	return sqrtf(dot(a, a));
}

vec3 vec3C(float inx, float iny, float inz) {
	vec3 out;
	out.x = inx;
	out.y = iny;
	out.z = inz;
	return out;
}
vec3 getnormalized(vec3 a) {
	float vectormag = mag(a);
	if (vectormag == 0.0f) {
		return vec3C(0.0f, 0.0f, 0.0f);
	}
	else {
		return vecdivs(a, vectormag);
	}
}
vec3 vecrcp(vec3 a) {
	return vec3C(1.0f / a.x, 1.0f / a.y, 1.0f / a.z);
}
vec3 vecmin(vec3 a, vec3 b) {
	return vec3C(fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z));
}
vec3 vecmax(vec3 a, vec3 b) {
	return vec3C(fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z));
}
vec3 cross(vec3 a, vec3 b) {
	vec3 out;
	out.x = a.y * b.z - a.z * b.y;
	out.y = a.z * b.x - a.x * b.z;
	out.z = a.x * b.y - a.y * b.x;
	return out;
}

typedef struct {
	float x;
	float y;
} vec2;
vec2 vec2C(float inx, float iny) {
	vec2 out;
	out.x = inx;
	out.y = iny;
	return out;
}
vec2 vecadd2(vec2 a, vec2 b) {
	vec2 out;
	out.x = a.x + b.x;
	out.y = a.y + b.y;
	return out;
}
vec2 vecsub2(vec2 a, vec2 b) {
	vec2 out;
	out.x = a.x - b.x;
	out.y = a.y - b.y;
	return out;
}
vec2 vecmuls2(vec2 a, float b) {
	vec2 out;
	out.x = a.x * b;
	out.y = a.y * b;
	return out;
}
vec2 vecdiv2(vec2 a, vec2 b) {
	vec2 out;
	out.x = a.x / b.x;
	out.y = a.y / b.y;
	return out;
}
vec2 vecdivs2(vec2 a, float b) {
	vec2 out;
	out.x = a.x / b;
	out.y = a.y / b;
	return out;
}
float vecmag2(vec2 a)
{
	return sqrtf(a.x * a.x + a.y * a.y);
}

typedef struct {
	float x, y, z, w;
} vec4;
vec4 vec4C(float xin, float yin, float zin, float win) {
	vec4 out;
	out.x = xin;
	out.y = yin;
	out.z = zin;
	out.w = win;
	return out;
}
vec4 vec4CS(vec3 a, float win) {
	vec4 out;
	out.x = a.x;
	out.y = a.y;
	out.z = a.z;
	out.w = win;
	return out;
}
float dot4(vec4 a, vec4 b) {
	float out;
	out = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	return out;
}

typedef struct {
	vec4 rows[4];
} mat44;
mat44 mat44C(vec4 r0, vec4 r1, vec4 r2, vec4 r3)
{
	mat44 out;
	out.rows[0] = r0;
	out.rows[1] = r1;
	out.rows[2] = r2;
	out.rows[3] = r3;
	return out;
}
mat44 matmatmul44(mat44 mata, mat44 matb) {
	mat44 matout;
	vec4 colvec0 = vec4C(matb.rows[0].x, matb.rows[1].x, matb.rows[2].x, matb.rows[3].x);
	vec4 colvec1 = vec4C(matb.rows[0].y, matb.rows[1].y, matb.rows[2].y, matb.rows[3].y);
	vec4 colvec2 = vec4C(matb.rows[0].z, matb.rows[1].z, matb.rows[2].z, matb.rows[3].z);
	vec4 colvec3 = vec4C(matb.rows[0].w, matb.rows[1].w, matb.rows[2].w, matb.rows[3].w);
	for (int row = 0; row < 4; ++row)
	{
		matout.rows[row].x = dot4(mata.rows[row], colvec0);
		matout.rows[row].y = dot4(mata.rows[row], colvec1);
		matout.rows[row].z = dot4(mata.rows[row], colvec2);
		matout.rows[row].w = dot4(mata.rows[row], colvec3);
	}
	return matout;
}
vec4 matmul44(mat44 mat, vec4 vec) {
	vec4 vecout;
	vecout.x = dot4(mat.rows[0], vec);
	vecout.y = dot4(mat.rows[1], vec);
	vecout.z = dot4(mat.rows[2], vec);
	vecout.w = dot4(mat.rows[3], vec);
	return vecout;
}

void swapi(int* a, int* b) {
	int temp = *a;
	*a = *b;
	*b = temp;
}
void swap(float* a, float* b) {
	float temp = *a;
	*a = *b;
	*b = temp;
}
void swap2(vec2* a, vec2* b) {
	vec2 temp = *a;
	*a = *b;
	*b = temp;
}
void swap3(vec3* a, vec3* b) {
	vec3 temp = *a;
	*a = *b;
	*b = temp;
}
void swapptr(void** a, void** b) {
	void* temp = *a;
	*a = *b;
	*b = temp;
}

int absi(int a) {
	return a < 0 ? -a : a;
}

typedef struct {
	float x;
	float y;
	float z;
	float uvx;
	float uvy;
	float uvz;
} VertexPUV;

typedef struct {
	vec3 a, b, c;
	vec3 auv, buv, cuv;
	float aw, bw, cw;
} TriangleUV;
TriangleUV TriangleUVC(
	vec3 ain,
	vec3 bin,
	vec3 cin,
	vec3 auvin,
	vec3 buvin,
	vec3 cuvin,
	float awin,
	float bwin,
	float cwin) {
	TriangleUV tri;
	tri.a = ain;
	tri.b = bin;
	tri.c = cin;
	tri.auv = auvin;
	tri.buv = buvin;
	tri.cuv = cuvin;
	tri.aw = awin;
	tri.bw = bwin;
	tri.cw = cwin;
	return tri;
}
void sortascending(TriangleUV* triangle) {
	if (triangle->a.y > triangle->b.y) {
		swap3(&triangle->a, &triangle->b);
		swap3(&triangle->auv, &triangle->buv);
		swap(&triangle->aw, &triangle->bw);
	}
	if (triangle->b.y > triangle->c.y) {
		swap3(&triangle->b, &triangle->c);
		swap3(&triangle->buv, &triangle->cuv);
		swap(&triangle->bw, &triangle->cw);
	}
	if (triangle->a.y > triangle->b.y) {
		swap3(&triangle->a, &triangle->b);
		swap3(&triangle->auv, &triangle->buv);
		swap(&triangle->aw, &triangle->bw);
	}
}
vec3 facenormal(TriangleUV triangles) {
	vec3 tricross = cross(vecsub(triangles.a, triangles.b),
		vecsub(triangles.c, triangles.a));
	return getnormalized(tricross);
}
typedef struct {
	vec3 a;
	vec3 b;
	vec3 uva;
	vec3 uvb;
	float wa;
	float wb;
} Edge3UV;
Edge3UV Edge3UVC(vec3 ain, vec3 bin, vec3 auvin, vec3 buvin, float wain, float wbin) {
	Edge3UV out;
	out.a = ain;
	out.b = bin;
	out.uva = auvin;
	out.uvb = buvin;
	out.wa = wain;
	out.wb = wbin;
	return out;
}

void rastertriangleuv(
	DrawMeshArgs* meshargs,
	TriangleUV tri);


unsigned short Color555To565(unsigned short color555)
{
	unsigned short r = (color555 >> 10) & 0x1f;
	unsigned short g = (color555 >> 5) & 0x1f;
	unsigned short b = (color555) & 0x1f;
	unsigned short color565 = r << 11;
	color565 |= g << 5 << 1;
	color565 |= b;
	return color565;
}

inline int clampi(int val, int min, int max) {
	int out = val;
	if (val < min) {
		out = min;
	}
	if (val > max) {
		out = max;
	}
	return out;
}

inline float clampf(float val, float min, float max) {
	float out = val;
	if (val < min) {
		out = min;
	}
	if (val > max) {
		out = max;
	}
	return out;
}

inline float minf(float a, float b) {
	return a < b ? a : b;
}

inline float maxf(float a, float b) {
	return a > b ? a : b;
}

inline int mini(int a, int b) {
	return a < b ? a : b;
}

inline int maxi(int a, int b) {
	return a > b ? a : b;
}

// Return signed shortest distance from point to plane, plane normal must be normalised
inline float vertexplanedistance(vec3 vertex, vec3 planepoint, vec3 planenormal) {
	return dot(planenormal, vertex) - dot(planenormal, planepoint);
};

inline vec3 vectorplaneintersect(vec3 plane_p, vec3 plane_n, vec3 lineStart, vec3 lineEnd, float* t)
{
	plane_n = getnormalized(plane_n);
	float plane_d = -(dot(plane_n, plane_p));
	float ad = dot(lineStart, plane_n);
	float bd = dot(lineEnd, plane_n);
	*t = (-plane_d - ad) / (bd - ad);
	vec3 lineStartToEnd = vecsub(lineEnd, lineStart);
	vec3 lineToIntersect = vecmuls(lineStartToEnd, *t);
	return vecadd(lineStart, lineToIntersect);
}

unsigned int GetTileIndex(
	struct gb_s* gb,
	int tileX,
	int tileY,
	int scrollX,
	int scrollY,
	bool bWindowMode)
{
	uint8_t bg_y, disp_x, bg_x, idx, py, px, t1, t2;
	uint16_t bg_map, tile;

	if (!bWindowMode)
	{
		tileX += scrollX;
		tileY += scrollY;
	}

	/* Calculate current background line to draw. Constant because
		 * this function draws only this one line each time it is
		 * called. */
	bg_y = tileY * 8;

	if (bWindowMode)
	{
		//tileY -= gbMinWindowLYPerFrame;
	}

	/* Get selected background map address for first tile
	 * corresponding to current line.
	 * 0x20 (32) is the width of a background tile, and the bit
	 * shift is to calculate the address. */
	if (!bWindowMode)
	{
		bg_map =
			((gb->hram_io[IO_LCDC] & LCDC_BG_MAP) ?
				VRAM_BMAP_2 : VRAM_BMAP_1)
			+ (bg_y >> 3) * 0x20;
	}
	else
	{
		bg_map =
			((gb->hram_io[IO_LCDC] & LCDC_WINDOW_MAP) ?
				VRAM_BMAP_2 : VRAM_BMAP_1)
			+ ((bg_y - gbMinWindowLYPerFrame) >> 3) * 0x20;
	}

	/* The displays (what the player sees) X coordinate, drawn right
	 * to left. */
	disp_x = tileX * 8;
	if (bWindowMode)
	{
		disp_x += gb->hram_io[IO_WX] + 7;
	}

	/* The X coordinate to begin drawing the background at. */
	bg_x = disp_x + 0; // scroll x as 0

	/* Get tile index for current background tile. */
	idx = gb->vram[bg_map + (bg_x >> 3)];

	unsigned char addressMode = gb->hram_io[IO_LCDC] & LCDC_TILE_SELECT; // LCDC
	if (bWindowMode)
	{
		addressMode = gbWinTileSelect;
	}
	/* Select addressing mode. */
	if (addressMode & LCDC_TILE_SELECT)
		tile = VRAM_TILES_1 + idx * 0x10;
	else
		tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;
	return tile;
}

unsigned short SampleTile(struct gb_s* gb, unsigned int tileIndex, float x, float y, bool* alphaClipTile)
{
	*alphaClipTile = false;
	unsigned int xInt = clampi(roundf(x * 7), 0, 7);
	unsigned int yInt = 7 - clampi(roundf(y * 7), 0, 7);

	unsigned int yOffset = yInt * 2;
	// Two bit planes for each row
	unsigned char tileBitPlane0 = gb->vram[tileIndex + yOffset];
	unsigned char tileBitPlane1 = gb->vram[tileIndex + yOffset + 1];
	unsigned char bitMask = 0x1;
	int paletteLookup = (tileBitPlane0 >> xInt) & bitMask;
	paletteLookup |= ((tileBitPlane1 >> xInt) & bitMask) << 1;
	if (paletteLookup == 0)
	{
		*alphaClipTile = true;
	}
	return Color555To565(gb->direct.priv->selected_palette[BG_PALLET][paletteLookup]);
}

unsigned short SampleOAM(
	struct gb_s* gb,
	int tileIndex,
	unsigned char attributes,
	float uvX,
	float uvY,
	bool* bTransparent)
{
	uint8_t ySize = (gb->hram_io[IO_LCDC] & LCDC_OBJ_SIZE ? 15 : 7);
	// handle x flip
	bool bXFlip = (attributes & OBJ_FLIP_X) != 0;
	bool bYFlip = (attributes & OBJ_FLIP_Y) != 0;

	unsigned int xInt = clampi(roundf(uvX * 7), 0, 7);
	unsigned int yInt = ySize - clampi(roundf(uvY * ySize), 0, ySize);

	if (bXFlip)
	{
		xInt = 7 - xInt;
	}
	if (bYFlip)
	{
		yInt = (gb->hram_io[IO_LCDC] & LCDC_OBJ_SIZE ? 15 : 7) - yInt;
	}

	unsigned int yOffset = yInt * 2;
	// Two bit planes for each row
	unsigned char tileBitPlane0 = gb->vram[tileIndex + yOffset];
	unsigned char tileBitPlane1 = gb->vram[tileIndex + yOffset + 1];
	unsigned char bitMask = 0x1;
	int paletteLookup;
	paletteLookup = (tileBitPlane0 >> xInt) & bitMask;
	paletteLookup |= ((tileBitPlane1 >> xInt) & bitMask) << 1;
	*bTransparent = paletteLookup == 0x0;
	// Select palette
	int objPalette = (attributes & OBJ_PALETTE) != 0 ? 1 : 0;
	return Color555To565(gb->direct.priv->selected_palette[objPalette][3 - paletteLookup]);
}

void GetSpriteProperties(
	struct gb_s* gb,
	uint8_t spriteIndex,
	float* xPosition,
	float* yPosition,
	int* tileIndex,
	uint8_t* attributes)
{
	uint8_t s = spriteIndex;
	/* Sprite Y position. */
	uint8_t OY = gb->oam[4 * s + 0];
	/* Sprite X position. */
	uint8_t OX = gb->oam[4 * s + 1];
	/* Sprite Tile/Pattern Number. */
	uint8_t OT = gb->oam[4 * s + 2]
		& (gb->hram_io[IO_LCDC] & LCDC_OBJ_SIZE ? 0xFE : 0xFF);
	/* Additional attributes. */
	uint8_t OF = gb->oam[4 * s + 3];

	/* Continue if sprite not visible. */
	//if (OX == 0 || OX >= 168)
		//continue;

	*tileIndex = VRAM_TILES_1 + OT * 0x10;
	*attributes = OF;
	//*xPosition = OX / 8.0;
	//*yPosition = (144 - OY / 8.0f + 16);
	*xPosition = OX;
	*yPosition = OY;
}

inline void drawscandepthuv(
	DrawMeshArgs* meshargs,
	int y,
	int x1,
	int x2,
	float z1,
	float z2,
	vec3 uva,
	vec3 uvb,
	float wa,
	float wb) {
	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
		vec3 tempuv = uva;
		uva = uvb;
		uvb = tempuv;
		float tempz = z1;
		z1 = z2;
		z2 = tempz;
		float tempw = wa;
		wa = wb;
		wb = tempw;
	}
	int x1c = clampi(x1, 0, width - 1);
	int x2c = clampi(x2, 0, width - 1);
	float totalwidth = (float)x2 - (float)x1;
	if (totalwidth <= 0.0f) {
		return;
	}
	for (int x = x1c; x <= x2c; ++x) {
		float t = ((float)x - (float)x1) / totalwidth;
		float z = z1 * ((float)1.0f - t) + z2 * t;
		float w = wa * ((float)1.0f - t) + wb * t;
		if (w == 0.0f) {
			continue;
		}
		if (meshargs->depth[y * height + x] > z / w) {
			// TODO flexible shader implementation should go here
			// Or write attributes to gbuffer
			vec3 uv = vecadd(vecmuls(uva, (1.0f - t)), vecmuls(uvb, t));
			bool bTileClip = false;
			unsigned short rgb565;
			if (meshargs->shaderstate.UseMeshColor) {
				rgb565 = meshargs->shaderstate.MeshColor;
			}
			else if (meshargs->shaderstate.UseVertexColor) {
				vec3 worlduv = vecdivs(uv, w);
				rgb565 = FloatTo565((float*)&worlduv);
			}
			else {
				vec2 uvWCorrect = vecdivs2(vec2C(uv.x, uv.y), w);
				if (meshargs->sampleBG)
				{
					rgb565 = SampleTile(meshargs->gb, meshargs->tileIndex, uvWCorrect.x, uvWCorrect.y, &bTileClip);
				}
				else
				{
					bool bTransparent = false;
					rgb565 = SampleOAM(meshargs->gb, meshargs->tileIndex, meshargs->attributes, uvWCorrect.x, uvWCorrect.y, &bTransparent);
					if (bTransparent)
					{
						continue;
					}
				}
			}
#if 1
			if (bTileClip && meshargs->blendstate.AlphaClipEnable)
			{
				continue;
			}
#else
			if (meshargs->blendstate.AlphaClipEnable &&
				meshargs->blendstate.AlphaClipColor == rgb565) {
				continue;
			}
#endif
			if (!(x >= 0 && x < width && y >= 0 && y < height)) {
				continue;
			}
			meshargs->depth[y * height + x] = z / w;
			meshargs->color[y * height + x] = rgb565;
		}
	}
}

// draw from top to bottom 
void fillBottomFlatTriangleuv(
	DrawMeshArgs* meshargs,
	TriangleUV tri,
	float slope0,
	float slope1) {
	vec3 v1 = tri.a;
	vec3 v2 = tri.b;
	vec3 v3 = tri.c;
	vec3 uv1 = tri.auv;
	vec3 uv2 = tri.buv;
	vec3 uv3 = tri.cuv;
	float w1 = tri.aw;
	float w2 = tri.bw;
	float w3 = tri.cw;

	if (v2.y - v1.y == 0.0f) {
		//return;
	}
	if (v3.y - v1.y == 0.0f) {
		//return;
	}

	float invslope1 = slope0;
	float invslope2 = slope1;

	float invdepthslope1 = (v2.z - v1.z) / (v2.y - v1.y);
	float invdepthslope2 = (v3.z - v1.z) / (v3.y - v1.y);

	vec3 invuvslope1 = vecdivs(vecsub(uv2, uv1), v2.y - v1.y);
	vec3 invuvslope2 = vecdivs(vecsub(uv3, uv1), v3.y - v1.y);

	float invwslope1 = (w2 - w1) / (v2.y - v1.y);
	float invwslope2 = (w3 - w1) / (v3.y - v1.y);

	float curx1 = v1.x;
	float curx2 = v1.x;

	float curz1 = v1.z;
	float curz2 = v1.z;

	vec3 curuv1 = uv1;
	vec3 curuv2 = uv1;

	float curw1 = w1;
	float curw2 = w1;

	float minx = minf(minf(v1.x, v2.x), v3.x);
	float maxx = maxf(maxf(v1.x, v2.x), v3.x);
	minx = floorf(minx);
	maxx = ceilf(maxx);

	v1.y = (int)clampi((int)v1.y, 0, (int)height - 1);
	v2.y = (int)clampi((int)v2.y, 0, (int)height - 1);

	for (int scanlineY = (int)floorf(v1.y); scanlineY <= (int)ceilf(v2.y); scanlineY++) {
		curx1 = clampf(curx1, (float)minx, (float)maxx);
		curx2 = clampf(curx2, (float)minx, (float)maxx);
		drawscandepthuv(meshargs, scanlineY, (int)curx1, (int)curx2, curz1, curz2, curuv1, curuv2, curw1, curw2);
		curx1 += invslope1;
		curx2 += invslope2;
		curz1 += invdepthslope1;
		curz2 += invdepthslope2;
		curuv1 = vecadd(curuv1, invuvslope1);
		curuv2 = vecadd(curuv2, invuvslope2);
		curw1 += invwslope1;
		curw2 += invwslope2;
	}
}
// draw from bottom to top 
void fillTopFlatTriangleuv(
	DrawMeshArgs* meshargs,
	TriangleUV tri,
	float slope0,
	float slope1) {
	vec3 v1 = tri.a;
	vec3 v2 = tri.b;
	vec3 v3 = tri.c;
	vec3 uv1 = tri.auv;
	vec3 uv2 = tri.buv;
	vec3 uv3 = tri.cuv;
	float w1 = tri.aw;
	float w2 = tri.bw;
	float w3 = tri.cw;

	if (v3.y - v1.y == 0.0f) {
		//return;
	}
	if (v3.y - v2.y == 0.0f) {
		//return;
	}

	float invslope1 = slope0;
	float invslope2 = slope1;

	float invdepthslope1 = (v3.z - v1.z) / (v3.y - v1.y);
	float invdepthslope2 = (v3.z - v2.z) / (v3.y - v2.y);

	vec3 invuvslope1 = vecdivs(vecsub(uv3, uv1), v3.y - v1.y);
	vec3 invuvslope2 = vecdivs(vecsub(uv3, uv2), v3.y - v2.y);

	float invwslope1 = (w3 - w1) / (v3.y - v1.y);
	float invwslope2 = (w3 - w2) / (v3.y - v2.y);

	float curx1 = v3.x;
	float curx2 = v3.x;

	float curz1 = v3.z;
	float curz2 = v3.z;

	vec3 curuv1 = uv3;
	vec3 curuv2 = uv3;

	float curw1 = w3;
	float curw2 = w3;

	float minx = minf(minf(v1.x, v2.x), v3.x);
	float maxx = maxf(maxf(v1.x, v2.x), v3.x);
	minx = floorf(minx);
	maxx = ceilf(maxx);

	v1.y = (int)clampi((int)v1.y, 0, (int)height - 1);
	v2.y = (int)clampi((int)v2.y, 0, (int)height - 1);

	for (int scanlineY = (int)ceilf(v3.y); scanlineY > (int)(floorf(v1.y)); scanlineY--) {
		curx1 = clampf(curx1, (float)minx, (float)maxx);
		curx2 = clampf(curx2, (float)minx, (float)maxx);

		drawscandepthuv(meshargs, scanlineY, (int)curx1, (int)curx2, curz1, curz2, curuv1, curuv2, curw1, curw2);
		curx1 -= invslope1;
		curx2 -= invslope2;
		curz1 -= invdepthslope1;
		curz2 -= invdepthslope2;
		curuv1 = vecsub(curuv1, invuvslope1);
		curuv2 = vecsub(curuv2, invuvslope2);
		curw1 -= invwslope1;
		curw2 -= invwslope2;
	}
}

inline float det2D(vec3 v0, vec3 v1)
{
	return (v0.x * v1.y - v0.y * v1.x);
}

float fmax3f(float a, float b, float c)
{
	return fmaxf(fmaxf(a, b), c);
}

float fmin3f(float a, float b, float c)
{
	return fminf(fminf(a, b), c);
}

// Bari check eq from TinyRaster
void bariFillTriangle(
	DrawMeshArgs* meshargs,
	TriangleUV tri) {

	vec3 v0 = tri.a;
	vec3 v1 = tri.b;
	vec3 v2 = tri.c;
	vec3 uv0 = tri.auv;
	vec3 uv1 = tri.buv;
	vec3 uv2 = tri.cuv;
	float w0 = tri.aw;
	float w1 = tri.bw;
	float w2 = tri.cw;

	v0.z /= w0;
	v1.z /= w1;
	v2.z /= w2;

	int xmin = 0;
	int xmax = width - 1;
	int ymin = 0;
	int ymax = height - 1;

	xmin = fmaxf(xmin, fmin3f(floorf(v0.x), floorf(v1.x), floorf(v2.x)));
	xmax = fminf(xmax, fmax3f(floorf(v0.x), floorf(v1.x), floorf(v2.x)));
	ymin = fmaxf(ymin, fmin3f(floorf(v0.y), floorf(v1.y), floorf(v2.y)));
	ymax = fminf(ymax, fmax3f(floorf(v0.y), floorf(v1.y), floorf(v2.y)));

	float det012 = det2D(vecsub(v1, v0), vecsub(v2, v0));

	bool ccw = det012 < 0.f;
	if (ccw) {
		swap3(&v1, &v2);
		swap(&w1, &w2);
		swap3(&uv1, &uv2);
		det012 = -det012;
	}

	for (int y = ymin; y <= ymax; ++y) {
		for (int x = xmin; x <= xmax; ++x) {
			vec3 p = vec3C(x + 0.5f, y + 0.5f, 0.f);

			float det01p = det2D(vecsub(v1, v0), vecsub(p, v0));
			float det12p = det2D(vecsub(v2, v1), vecsub(p, v1));
			float det20p = det2D(vecsub(v0, v2), vecsub(p, v2));

			if (det01p < 0.f || det12p < 0.f || det20p < 0.f) {
				continue;
			}

			float l0 = det12p / det012 * w0;
			float l1 = det20p / det012 * w1;
			float l2 = det01p / det012 * w2;

			float lsum = l0 + l1 + l2;

			l0 /= lsum;
			l1 /= lsum;
			l2 /= lsum;

			// TODO undo w application in clipping stage, use bari weights to lerp each attribute
			float texcoord0 = l0 * uv0.x / w0 + l1 * uv1.x / w1 + l2 * uv2.x / w2;
			float texcoord1 = l0 * uv0.y / w0 + l1 * uv1.y / w1 + l2 * uv2.y / w2;

			vec3 ndc_position = vecadd(vecadd(vecmuls(v0, l0), vecmuls(v1, l1)), vecmuls(v2, l2));

			float depth = ndc_position.z;

			if (meshargs->depth[y * height + x] > depth)
			{
				bool bAlphaClipTile = false;
				unsigned short rgb565;
				if (meshargs->shaderstate.UseMeshColor) {
					rgb565 = meshargs->shaderstate.MeshColor;
				}
				else if (meshargs->shaderstate.UseVertexColor) {
					vec3 worlduv = vec3C(texcoord0, texcoord1, 0.0f);
					rgb565 = FloatTo565((float*)&worlduv);
				}
				else {
					if (meshargs->sampleBG)
					{
						rgb565 = SampleTile(meshargs->gb, meshargs->tileIndex, texcoord0, texcoord1, &bAlphaClipTile);
					}
					else
					{
						bool bTransparent = false;
						rgb565 = SampleOAM(meshargs->gb, meshargs->tileIndex, meshargs->attributes, texcoord0, texcoord1, &bTransparent);
						if (bTransparent)
						{
							continue;
						}
					}
				}
#if 1
				if (meshargs->blendstate.AlphaClipEnable && bAlphaClipTile)
				{
					continue;
				}
#else
				if (meshargs->blendstate.AlphaClipEnable &&
					meshargs->blendstate.AlphaClipColor == rgb565) {
					continue;
				}
#endif
				meshargs->depth[y * height + x] = depth;
				meshargs->color[y * height + x] = rgb565;
			}
		}
	}
}

typedef enum {
	NONE,
	A,
	B,
	AB,
} ClipCode;
// fill a triangle using the standard fill algorithm filling top and bottom triangles
// convert to Brensenham for interger math speedup
// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
inline void rastertriangleuv(
	DrawMeshArgs* meshargs,
	TriangleUV tri) {
	// sort verts in y order where the highest position y will be vetexa
	if (!bUseBariFill) {
		sortascending(&tri);

		vec3 a = tri.a;
		a.x = floorf(a.x);
		a.y = floorf(a.y);
		vec3 b = tri.b;
		b.x = floorf(b.x);
		b.y = floorf(b.y);
		vec3 c = tri.c;
		c.x = floorf(c.x);
		c.y = floorf(c.y);
		vec3 auv = tri.auv;
		vec3 buv = tri.buv;
		vec3 cuv = tri.cuv;
		float aw = tri.aw;
		float bw = tri.bw;
		float cw = tri.cw;

		float abSlope = (b.x - a.x) / (b.y - a.y);
		float acSlope = (c.x - a.x) / (c.y - a.y);
		float bcSlope = (c.x - b.x) / (c.y - b.y);
		// Flat case bottom
		if (tri.b.y == tri.c.y) {
			fillBottomFlatTriangleuv(meshargs, tri, abSlope, acSlope);
		}
		// Flat cast top
		else if (tri.a.y == tri.b.y) {
			fillTopFlatTriangleuv(meshargs, tri, acSlope, bcSlope);
		}
		else {
			float ylerp = (b.y - a.y) / (c.y - a.y);
			float depthinterp = (a.z + ylerp * (c.z - a.z));
			vec3 d = vec3C(
				a.x + ylerp * (c.x - a.x),
				b.y,
				depthinterp);

			vec3 duv = vec3C(
				tri.auv.x * (1.0f - ylerp) + tri.cuv.x * ylerp,
				tri.auv.y * (1.0f - ylerp) + tri.cuv.y * ylerp,
				tri.auv.z * (1.0f - ylerp) + tri.cuv.z * ylerp);
			d.x = floorf(d.x);
			d.y = floorf(d.y);
			float dw = tri.aw * (1.0f - ylerp) + tri.cw * ylerp;
			TriangleUV tria = TriangleUVC(a, b, d, auv, buv, duv, aw, bw, dw);
			fillBottomFlatTriangleuv(meshargs, tria, abSlope, acSlope);
			TriangleUV trib = TriangleUVC(b, d, c, buv, duv, cuv, bw, dw, cw);
			fillTopFlatTriangleuv(meshargs, trib, bcSlope, acSlope);
		}
	}
	else {
		bariFillTriangle(meshargs, tri);
	}
}

inline Edge3UV clipplanecodeduv(
	Edge3UV edge,
	vec3 planepoint,
	vec3 planenormal,
	ClipCode* clipcode) {
	*clipcode = AB;
	Edge3UV edgeout = edge;
	float d0 = vertexplanedistance(edge.a, planepoint, planenormal);
	float d1 = vertexplanedistance(edge.b, planepoint, planenormal);
	bool intersect = false;
	bool flippededge = false;
	if ((d0 >= 0.0f) ^ (d1 >= 0.0f)) {
		intersect = true;
		if (d1 >= 0.0f) {
			flippededge = true;
			edgeout.a = edge.b;
			edgeout.b = edge.a;
			edgeout.uva = edge.uvb;
			edgeout.uvb = edge.uva;
			edgeout.wa = edge.wb;
			edgeout.wb = edge.wa;
		}
	}
	if ((d0 < 0.0f) && (d1 < 0.0f)) {
		*clipcode = NONE;
	}
	if (intersect) {
		float t = 0.0f;
		vec3 intersect0 = vectorplaneintersect(planepoint, planenormal, edgeout.a, edgeout.b, &t);
		if (t >= 0.0f && t <= 1.0f) {
			*clipcode = flippededge ? B : A;
			edgeout.b = intersect0;
			edgeout.uvb = vecadd(vecmuls(edgeout.uva, (1.0f - t)), vecmuls(edgeout.uvb, t));
			edgeout.wb = edgeout.wa * (1.0f - t) + edgeout.wb * t;
		}
	}
	return edgeout;
}

// Sutherland Hodgman clipping algorithm
// there are other algorithms with less comparison that could be used instead
// this algorithm can work with polygons not just triangles
#define MAX_CLIPPED_VERTS 16 // probably not actually the actual max triangles that can be generated
// Passed verts initialized in camera space
void triangleclipuv(
	mat44 projmat,
	vec3* passedverts,
	vec3* passeduvs,
	float* passedws,
	int* numpassedverts) {
#define NUM_PLANES 5
	const vec3 planepoints[NUM_PLANES] = {
		vec3C(0.0f, 0.0f, near), // near
		vec3C(0.0f, 0.0f, 0.0f), // bottom
		vec3C(1.0f, 1.0f, 0.0f), // top
		vec3C(0.0f, 0.0f, 0.0f), // left
		vec3C(1.0f, 1.0f, 0.0f), // right
	};
	const vec3 planenormals[NUM_PLANES] = {
		vec3C(0.0f, 0.0f, 1.0f), // near
		vec3C(0.0f, 1.0f, 0.0f),
		vec3C(0.0f, -1.0f, 0.0f),
		vec3C(1.0f, 0.0f, 0.0f),
		vec3C(-1.0f, 0.0f, 0.0f),
	};

	vec3 outputpassedvertsarray[MAX_CLIPPED_VERTS];
	vec3 outputpasseduvsarray[MAX_CLIPPED_VERTS];
	float outputpassedwsarray[MAX_CLIPPED_VERTS];
	vec3* outputpassedverts = outputpassedvertsarray;
	vec3* outputpasseduvs = outputpasseduvsarray;
	float* outputpassedws = outputpassedwsarray;
	// This should be done in clip space but is split between projection and camera space for now
	// clip the near plane in camera space to simplify
	{
		int numoutpassedverts = *numpassedverts;
		// reset passed verts for the clipping loops
		*numpassedverts = 0;
		for (int vertexi = 0; vertexi < numoutpassedverts; ++vertexi) {
			Edge3UV visitededge = Edge3UVC(
				passedverts[vertexi],
				passedverts[(vertexi + 1) % numoutpassedverts],
				passeduvs[vertexi],
				passeduvs[(vertexi + 1) % numoutpassedverts],
				1.0f,  // wa
				1.0f); // wb
			ClipCode clipcode = NONE;
			Edge3UV clippededge = clipplanecodeduv(visitededge, planepoints[0], planenormals[0], &clipcode);
			// returns a clip code that determines which verts if any should pass the ith clip plane
			switch (clipcode) {
			case NONE: // edge outside the plane, add no verts
				break;
			case AB:   // edge entirely inside the frustum, add only first vertex
				// as the second vertex of the edge will be added in the next or previous iterration
				outputpasseduvs[*numpassedverts] = clippededge.uva;
				outputpassedverts[(*numpassedverts)++] = clippededge.a;
				break;
			case A: // edge starting inside, add both clipped verts
				outputpasseduvs[*numpassedverts] = clippededge.uva;
				outputpassedverts[(*numpassedverts)++] = clippededge.a;
				outputpasseduvs[*numpassedverts] = clippededge.uvb;
				outputpassedverts[(*numpassedverts)++] = clippededge.b;
				break;
			case B: // edge starting outside, record the vert on the plane
				// the other vert will be added in another iterration
				outputpasseduvs[*numpassedverts] = clippededge.uvb;
				outputpassedverts[(*numpassedverts)++] = clippededge.b;
				break;
			}
		}
		swapptr((void**)&passedverts, (void**)&outputpassedverts);
		swapptr((void**)&passeduvs, (void**)&outputpasseduvs);
	}
	if (*numpassedverts <= 0) {
		return;
	}

	bool hasoutside = false;
	//bool hasinside = false;
	// Transform the remaining triangles to 0-1 clip space
	for (int vertexi = 0; vertexi < *numpassedverts; ++vertexi) {
		vec4 vecproj = matmul44(projmat, vec4CS(passedverts[vertexi], 1.0f));
		vec3 vecscreen = vecdivs(vec3C(vecproj.x, vecproj.y, vecproj.z), vecproj.w);
		float x0 = (1.0f - (vecscreen.x + 1.0f) * 0.5f);
		float y0 = (1.0f - (vecscreen.y + 1.0f) * 0.5f);
		vecscreen.x = x0;
		vecscreen.y = y0;
		passedverts[vertexi] = vecscreen;
		passeduvs[vertexi] = vecdivs(passeduvs[vertexi], vecproj.w);
		passedws[vertexi] = 1.0f / vecproj.w;
		// check only integer bits for any point outside the 0..1 clip which requires more clip testing
		if ((vecscreen.x < 0.0f || vecscreen.y < 0.0f) ||
			(vecscreen.x > 1.0f || vecscreen.y > 1.0f)) {
			hasoutside = true;
		}
		// check for any point inside the 0..1 clip with fraction bits 
		// 
		if ((vecscreen.x >= 0.0f && vecscreen.y >= 0.0f) &&
			(vecscreen.x <= 1.0f && vecscreen.y <= 1.0f)) {
			//hasinside = true;
		}
	}

	// area of 2d triangle in screen space after perspective divide
	// if 2 triangles are created from the near clip they should shader the same handedness
	// TODO program ccw in some render state object
	bool useccw = true;
	float sign = useccw ? 1.0f : -1.0f;
	float area = (passedverts[0].x - passedverts[1].x) * (passedverts[0].y - passedverts[2].y) -
		(passedverts[0].x - passedverts[2].x) * (passedverts[0].y - passedverts[1].y);

	// Backface culling
	if ((sign * area < 0.0f)) {
		//*numpassedverts = 0;
		//return;
	}

	// fast path trivial accept
	if (!hasoutside) {
		swapptr((void**)&passedverts, (void**)&outputpassedverts);
		swapptr((void**)&passeduvs, (void**)&outputpasseduvs);
		swapptr((void**)&passedws, (void**)&outputpassedws);
		memcpy(passedverts, outputpassedverts, sizeof(vec3) * *numpassedverts);
		memcpy(passeduvs, outputpasseduvs, sizeof(vec3) * *numpassedverts);
		memcpy(passedws, outputpassedws, sizeof(float) * *numpassedverts);
		// transform the remaining triangles to screen space
		for (int vertexi = 0; vertexi < *numpassedverts; ++vertexi) {
			vec3 vecscreen = passedverts[vertexi];
			//float x0 = (1.0f - (vecscreen.x + 1.0f) * 0.5f);
			//float y0 = (1.0f - (vecscreen.y + 1.0f) * 0.5f);
			float x0 = vecscreen.x * (float)width;
			float y0 = vecscreen.y * (float)height;
			vecscreen.x = x0;
			vecscreen.y = y0;
			passedverts[vertexi] = vecscreen;
		}
		return;
	}
#if 0
	// fast path trivial reject
	if (!hasinside) {
		*numpassedverts = 0;
		return;
	}
#endif

	// clip against 4 screen sides
	for (int planei = 1; planei < NUM_PLANES; ++planei) {
		int numoutpassedverts = *numpassedverts;
		// reset passed verts for the clipping loops
		*numpassedverts = 0;
		// for each pair of verts in order, check if they cross the axis of the plane
		// if neither vertex is inside the axis leave both verts from the list
		// if both verts are inside append both when they are the first vert in an edge
		// if only one vert is inside and another is outside append the intersected vert
		// when visiting some edges up to 2 verts are added which adds triangles to the the pass list
		// it is possible for many triangles to be generated from one initial triangle by 
		// clipping between 1 and 5 total planes
		for (int vertexi = 0; vertexi < numoutpassedverts; ++vertexi) {
			Edge3UV visitededge = Edge3UVC(
				passedverts[vertexi],
				passedverts[(vertexi + 1) % numoutpassedverts],
				passeduvs[vertexi],
				passeduvs[(vertexi + 1) % numoutpassedverts],
				passedws[vertexi],
				passedws[(vertexi + 1) % numoutpassedverts]);
			ClipCode clipcode = NONE;
			Edge3UV clippededge = clipplanecodeduv(visitededge, planepoints[planei], planenormals[planei], &clipcode);
			// returns a clip code that determines which verts if any should pass the ith clip plane
			switch (clipcode) {
			case NONE: // edge outside the plane, add no verts
				break;
			case AB:   // edge entirely inside the frustum, add only first vertex
				// as the second vertex of the edge will be added in the next or previous iterration
				outputpassedws[*numpassedverts] = clippededge.wa;
				outputpasseduvs[*numpassedverts] = clippededge.uva;
				outputpassedverts[(*numpassedverts)++] = clippededge.a;
				break;
			case A: // edge starting inside, add both clipped verts
				outputpassedws[*numpassedverts] = clippededge.wa;
				outputpasseduvs[*numpassedverts] = clippededge.uva;
				outputpassedverts[(*numpassedverts)++] = clippededge.a;
				outputpassedws[*numpassedverts] = clippededge.wb;
				outputpasseduvs[*numpassedverts] = clippededge.uvb;
				outputpassedverts[(*numpassedverts)++] = clippededge.b;
				break;
			case B: // edge starting outside, record the vert on the plane
				// the other vert will be added in another iterration
				outputpassedws[*numpassedverts] = clippededge.wb;
				outputpasseduvs[*numpassedverts] = clippededge.uvb;
				outputpassedverts[(*numpassedverts)++] = clippededge.b;
				break;
			}
		}
		if (*numpassedverts <= 0) {
			return;
		}
		swapptr((void**)&passedverts, (void**)&outputpassedverts);
		swapptr((void**)&passeduvs, (void**)&outputpasseduvs);
		swapptr((void**)&passedws, (void**)&outputpassedws);
	}
	swapptr((void**)&passedverts, (void**)&outputpassedverts);
	swapptr((void**)&passeduvs, (void**)&outputpasseduvs);
	swapptr((void**)&passedws, (void**)&outputpassedws);
	memcpy(passedverts, outputpassedverts, sizeof(vec3) * *numpassedverts);
	memcpy(passeduvs, outputpasseduvs, sizeof(vec3) * *numpassedverts);
	memcpy(passedws, outputpassedws, sizeof(float) * *numpassedverts);
	// transform the remaining triangles to screen space
	for (int vertexi = 0; vertexi < *numpassedverts; ++vertexi) {
		vec3 vecscreen = passedverts[vertexi];
		//float x0 = (1.0f - (vecscreen.x + 1.0f) * 0.5f);
		//float y0 = (1.0f - (vecscreen.y + 1.0f) * 0.5f);
		float x0 = vecscreen.x * (float)width;
		float y0 = vecscreen.y * (float)height;
		vecscreen.x = x0;
		vecscreen.y = y0;
		passedverts[vertexi] = vecscreen;
	}
}

void rastermesh(DrawMeshArgs* meshargs) {
	rastermeshoffset(meshargs, 0x0);
}

void rastermeshoffset(DrawMeshArgs* meshargs, float* positionOffset) {
#if RP2
#else
	// Set draw color to black
	SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
#endif
	int numtriangles = meshargs->numverts;
	VertexPUV* vertices = (VertexPUV*)meshargs->vertices;

	for (int vertind = 0; vertind < numtriangles; vertind += 3) {

		vec3 playerpos = vec3C(
			meshargs->cameraposition[0],
			meshargs->cameraposition[1],
			meshargs->cameraposition[2]);

		vec3 va = vec3C(vertices[vertind + 0].x, vertices[vertind + 0].y, vertices[vertind + 0].z);
		vec3 vb = vec3C(vertices[vertind + 1].x, vertices[vertind + 1].y, vertices[vertind + 1].z);
		vec3 vc = vec3C(vertices[vertind + 2].x, vertices[vertind + 2].y, vertices[vertind + 2].z);

		if (positionOffset) {
			vec3 offset = vec3C(positionOffset[0], positionOffset[1], positionOffset[2]);
			va = vecadd(va, offset);
			vb = vecadd(vb, offset);
			vc = vecadd(vc, offset);
		}

		vec3 diff0 = vecneg(vecsub(playerpos, va));
		vec3 diff1 = vecneg(vecsub(playerpos, vb));
		vec3 diff2 = vecneg(vecsub(playerpos, vc));
		const float farsquared = far * far;
		bool farcull =
			dot(diff0, diff0) > farsquared &&
			dot(diff1, diff1) > farsquared &&
			dot(diff2, diff2) > farsquared;
		// far cull without clipping
		if (farcull && farclip) {
			//continue;
		}

		mat44 viewmatrix;
		memcpy(&viewmatrix, meshargs->viewmatrix, sizeof(mat44));
		//const vec3 planepoint = vec3C(0.0f, 0.0f, near);
		//const vec3 planenormal = vec3C(0.0f, 0.0f, 1.0f);
		vec4 viewtria = matmul44(viewmatrix, vec4C(va.x, va.y, va.z, 1.0f));
		vec4 viewtrib = matmul44(viewmatrix, vec4C(vb.x, vb.y, vb.z, 1.0f));
		vec4 viewtric = matmul44(viewmatrix, vec4C(vc.x, vc.y, vc.z, 1.0f));
		vec3 uva = vec3C(vertices[vertind + 0].uvx, vertices[vertind + 0].uvy, vertices[vertind + 0].uvz);
		vec3 uvb = vec3C(vertices[vertind + 1].uvx, vertices[vertind + 1].uvy, vertices[vertind + 1].uvz);
		vec3 uvc = vec3C(vertices[vertind + 2].uvx, vertices[vertind + 2].uvy, vertices[vertind + 2].uvz);

		// fixed point starts here
		vec3 passedverts[MAX_CLIPPED_VERTS];
		vec3 passeduvs[MAX_CLIPPED_VERTS];
		float passedws[MAX_CLIPPED_VERTS];
		int numpassedverts = 3;
		passedverts[0] = vec3C(viewtria.x, viewtria.y, viewtria.z);
		passedverts[1] = vec3C(viewtrib.x, viewtrib.y, viewtrib.z);
		passedverts[2] = vec3C(viewtric.x, viewtric.y, viewtric.z);
		passeduvs[0] = vec3C(uva.x, uva.y, uva.z);
		passeduvs[1] = vec3C(uvb.x, uvb.y, uvb.z);
		passeduvs[2] = vec3C(uvc.x, uvc.y, uvc.z);
		passedws[0] = 1.0f;
		passedws[1] = 1.0f;
		passedws[2] = 1.0f;

		mat44 projectionmatrix;
		memcpy(&projectionmatrix, meshargs->projectionmatrix, sizeof(mat44));
		// 5 sided triangle clip
		triangleclipuv(projectionmatrix, &passedverts[0], &passeduvs[0], &passedws[0], &numpassedverts);
		// fill triangles
		for (int i = 0; i < numpassedverts - 2; ++i) {
			// draw triangle fan
			TriangleUV tri = TriangleUVC(
				passedverts[0],
				passedverts[i + 1],
				passedverts[i + 2],
				passeduvs[0],
				passeduvs[i + 1],
				passeduvs[i + 2],
				passedws[0],
				passedws[i + 1],
				passedws[i + 2]);
			rastertriangleuv(
				meshargs,
				tri);
		}
	}
}

#if !RP2

typedef struct {
	unsigned int numvertices;
	float* vertices;
} Mesh;


unsigned short colorbuffer[width * height * screenmultiple];
float depthbuffer[width * height * screenmultiple];

void GetMeshArgs(DrawMeshArgs* meshargs, vec3* position, mat44* projmat, mat44* viewmat, Mesh* mesh) {
	meshargs->color = colorbuffer;
	meshargs->depth = depthbuffer;
	meshargs->shaderstate.DrawGrid = false;
	meshargs->shaderstate.UseMeshColor = false;
	meshargs->shaderstate.UseVertexColor = false;
	meshargs->cameraposition = (float*)position;
	meshargs->projectionmatrix = (float*)projmat;
	meshargs->viewmatrix = (float*)viewmat;
	meshargs->vertices = mesh->vertices;
	meshargs->numverts = mesh->numvertices;
}

#define TILE_MAP_WIDTH (256 / 8)
#define TILE_MAP_HEIGHT (256 / 8)
#define LEVEL_HEIGHT 2.0f
#define SPRITE_HEIGHT 2.0f
#define STANDUP_SPRITE_HEIGHT 1.0f
Mesh tileMesh;
VertexPUV verts[6];
Mesh frontMesh;
VertexPUV frontVerts[6];
VertexPUV spriteVerts[6];
VertexPUV rampUp[6];
VertexPUV rampDown[6];
VertexPUV rampLeft[6];
VertexPUV rampRight[6];
VertexPUV standupSprite[6];
int selectedTileX;
int selectedTileY;
int numEntries;
void InitRaster()
{
	selectedTileX = 0;
	selectedTileY = 0;
	numEntries = 0;
#if !RP2
	LoadTileData("C:\\Peanut-GB\\LoZTileData.txt");
#endif
	tileMesh.numvertices = 6;
	tileMesh.vertices = verts;
	verts[0].x = 0.0f;
	verts[0].y = 0.0f;
	verts[0].z = 0.0f;
	verts[1].x = 1.0f;
	verts[1].y = 0.0f;
	verts[1].z = 0.0f;
	verts[2].x = 1.0f;
	verts[2].y = 1.0f;
	verts[2].z = 0.0f;
	verts[0].uvx = 0.0f;
	verts[0].uvy = 0.0f;
	verts[1].uvx = 1.0f;
	verts[1].uvy = 0.0f;
	verts[2].uvx = 1.0f;
	verts[2].uvy = 1.0f;

	verts[3].x = 0.0f;
	verts[3].y = 0.0f;
	verts[3].z = 0.0f;
	verts[3].uvx = 0.0f;
	verts[3].uvy = 0.0f;
	verts[4].x = 0.0f;
	verts[4].y = 1.0f;
	verts[4].z = 0.0f;
	verts[4].uvx = 0.0f;
	verts[4].uvy = 1.0f;
	verts[5].x = 1.0f;
	verts[5].y = 1.0f;
	verts[5].z = 0.0f;
	verts[5].uvx = 1.0f;
	verts[5].uvy = 1.0f;

	frontVerts[0].x = 0.0f;
	frontVerts[0].y = 0.0f;
	frontVerts[0].z = 0.0f;
	frontVerts[1].x = 1.0f;
	frontVerts[1].y = 0.0f;
	frontVerts[1].z = 0.0f;
	frontVerts[2].x = 1.0f;
	frontVerts[2].y = 0.0f;
	frontVerts[2].z = LEVEL_HEIGHT;
	frontVerts[0].uvx = 0.0f;
	frontVerts[0].uvy = 0.0f;
	frontVerts[1].uvx = 1.0f;
	frontVerts[1].uvy = 0.0f;
	frontVerts[2].uvx = 1.0f;
	frontVerts[2].uvy = 1.0f;

	frontVerts[3].x = 0.0f;
	frontVerts[3].y = 0.0f;
	frontVerts[3].z = 0.0f;
	frontVerts[3].uvx = 0.0f;
	frontVerts[3].uvy = 0.0f;
	frontVerts[4].x = 0.0f;
	frontVerts[4].y = 0.0f;
	frontVerts[4].z = LEVEL_HEIGHT;
	frontVerts[4].uvx = 0.0f;
	frontVerts[4].uvy = 1.0f;
	frontVerts[5].x = 1.0f;
	frontVerts[5].y = 0.0f;
	frontVerts[5].z = LEVEL_HEIGHT;
	frontVerts[5].uvx = 1.0f;
	frontVerts[5].uvy = 1.0f;

	spriteVerts[0].x = 0.0f;
	spriteVerts[0].y = 0.0f;
	spriteVerts[0].z = 0.0f;
	spriteVerts[1].x = 1.0f;
	spriteVerts[1].y = 0.0f;
	spriteVerts[1].z = 0.0f;
	spriteVerts[2].x = 1.0f;
	spriteVerts[2].y = SPRITE_HEIGHT;
	spriteVerts[2].z = -SPRITE_HEIGHT;
	spriteVerts[0].uvx = 0.0f;
	spriteVerts[0].uvy = 0.0f;
	spriteVerts[1].uvx = 1.0f;
	spriteVerts[1].uvy = 0.0f;
	spriteVerts[2].uvx = 1.0f;
	spriteVerts[2].uvy = 1.0f;

	spriteVerts[3].x = 0.0f;
	spriteVerts[3].y = 0.0f;
	spriteVerts[3].z = 0.0f;
	spriteVerts[3].uvx = 0.0f;
	spriteVerts[3].uvy = 0.0f;
	spriteVerts[4].x = 0.0f;
	spriteVerts[4].y = SPRITE_HEIGHT;
	spriteVerts[4].z = -SPRITE_HEIGHT;
	spriteVerts[4].uvx = 0.0f;
	spriteVerts[4].uvy = 1.0f;
	spriteVerts[5].x = 1.0f;
	spriteVerts[5].y = SPRITE_HEIGHT;
	spriteVerts[5].z = -SPRITE_HEIGHT;
	spriteVerts[5].uvx = 1.0f;
	spriteVerts[5].uvy = 1.0f;

	rampUp[0].x = 0.0f;
	rampUp[0].y = 0.0f;
	rampUp[0].z = 0.0f;
	rampUp[1].x = 1.0f;
	rampUp[1].y = 0.0f;
	rampUp[1].z = 0.0f;
	rampUp[2].x = 1.0f;
	rampUp[2].y = 1.0f;
	rampUp[2].z = -LEVEL_HEIGHT;
	rampUp[0].uvx = 0.0f;
	rampUp[0].uvy = 0.0f;
	rampUp[1].uvx = 1.0f;
	rampUp[1].uvy = 0.0f;
	rampUp[2].uvx = 1.0f;
	rampUp[2].uvy = 1.0f;
	
	rampUp[3].x = 0.0f;
	rampUp[3].y = 0.0f;
	rampUp[3].z = 0.0f;
	rampUp[3].uvx = 0.0f;
	rampUp[3].uvy = 0.0f;
	rampUp[4].x = 0.0f;
	rampUp[4].y = 1.0f;
	rampUp[4].z = -LEVEL_HEIGHT;
	rampUp[4].uvx = 0.0f;
	rampUp[4].uvy = 1.0f;
	rampUp[5].x = 1.0f;
	rampUp[5].y = 1.0f;
	rampUp[5].z = -LEVEL_HEIGHT;
	rampUp[5].uvx = 1.0f;
	rampUp[5].uvy = 1.0f;

	rampDown[0].x = 0.0f;
	rampDown[0].y = 0.0f;
	rampDown[0].z = -LEVEL_HEIGHT;
	rampDown[1].x = 1.0f;
	rampDown[1].y = 0.0f;
	rampDown[1].z = -LEVEL_HEIGHT;
	rampDown[2].x = 1.0f;
	rampDown[2].y = 1.0f;
	rampDown[2].z = 0.0f;
	rampDown[0].uvx = 0.0f;
	rampDown[0].uvy = 0.0f;
	rampDown[1].uvx = 1.0f;
	rampDown[1].uvy = 0.0f;
	rampDown[2].uvx = 1.0f;
	rampDown[2].uvy = 1.0f;
	
	rampDown[3].x = 0.0f;
	rampDown[3].y = 0.0f;
	rampDown[3].z = -LEVEL_HEIGHT;
	rampDown[3].uvx = 0.0f;
	rampDown[3].uvy = 0.0f;
	rampDown[4].x = 0.0f;
	rampDown[4].y = 1.0f;
	rampDown[4].z = 0.0f;
	rampDown[4].uvx = 0.0f;
	rampDown[4].uvy = 1.0f;
	rampDown[5].x = 1.0f;
	rampDown[5].y = 1.0f;
	rampDown[5].z = 0.0f;
	rampDown[5].uvx = 1.0f;
	rampDown[5].uvy = 1.0f;

	rampLeft[0].x = 0.0f;
	rampLeft[0].y = 0.0f;
	rampLeft[0].z = 0.0f;
	rampLeft[1].x = 1.0f;
	rampLeft[1].y = 0.0f;
	rampLeft[1].z = -LEVEL_HEIGHT;
	rampLeft[2].x = 1.0f;
	rampLeft[2].y = 1.0f;
	rampLeft[2].z = -LEVEL_HEIGHT;
	rampLeft[0].uvx = 0.0f;
	rampLeft[0].uvy = 0.0f;
	rampLeft[1].uvx = 1.0f;
	rampLeft[1].uvy = 0.0f;
	rampLeft[2].uvx = 1.0f;
	rampLeft[2].uvy = 1.0f;

	rampLeft[3].x = 0.0f;
	rampLeft[3].y = 0.0f;
	rampLeft[3].z = 0.0f;
	rampLeft[3].uvx = 0.0f;
	rampLeft[3].uvy = 0.0f;
	rampLeft[4].x = 0.0f;
	rampLeft[4].y = 1.0f;
	rampLeft[4].z = 0.0f;
	rampLeft[4].uvx = 0.0f;
	rampLeft[4].uvy = 1.0f;
	rampLeft[5].x = 1.0f;
	rampLeft[5].y = 1.0f;
	rampLeft[5].z = -LEVEL_HEIGHT;
	rampLeft[5].uvx = 1.0f;
	rampLeft[5].uvy = 1.0f;

	rampRight[0].x = 0.0f;
	rampRight[0].y = 0.0f;
	rampRight[0].z = -LEVEL_HEIGHT;
	rampRight[1].x = 1.0f;
	rampRight[1].y = 0.0f;
	rampRight[1].z = 0.0f;
	rampRight[2].x = 1.0f;
	rampRight[2].y = 1.0f;
	rampRight[2].z = 0.0f;
	rampRight[0].uvx = 0.0f;
	rampRight[0].uvy = 0.0f;
	rampRight[1].uvx = 1.0f;
	rampRight[1].uvy = 0.0f;
	rampRight[2].uvx = 1.0f;
	rampRight[2].uvy = 1.0f;
	
	rampRight[3].x = 0.0f;
	rampRight[3].y = 0.0f;
	rampRight[3].z = -LEVEL_HEIGHT;
	rampRight[3].uvx = 0.0f;
	rampRight[3].uvy = 0.0f;
	rampRight[4].x = 0.0f;
	rampRight[4].y = 1.0f;
	rampRight[4].z = -LEVEL_HEIGHT;
	rampRight[4].uvx = 0.0f;
	rampRight[4].uvy = 1.0f;
	rampRight[5].x = 1.0f;
	rampRight[5].y = 1.0f;
	rampRight[5].z = 0.0f;
	rampRight[5].uvx = 1.0f;
	rampRight[5].uvy = 1.0f;

	standupSprite[0].x = 0.0f;
	standupSprite[0].y = 0.0f;
	standupSprite[0].z = 0.0f;
	standupSprite[1].x = 1.0f;
	standupSprite[1].y = 0.0f;
	standupSprite[1].z = 0.0f;
	standupSprite[2].x = 1.0f;
	standupSprite[2].y = 1.0f;
	standupSprite[2].z = -STANDUP_SPRITE_HEIGHT;
	standupSprite[0].uvx = 0.0f;
	standupSprite[0].uvy = 0.0f;
	standupSprite[1].uvx = 1.0f;
	standupSprite[1].uvy = 0.0f;
	standupSprite[2].uvx = 1.0f;
	standupSprite[2].uvy = 1.0f;

	standupSprite[3].x = 0.0f;
	standupSprite[3].y = 0.0f;
	standupSprite[3].z = 0.0f;
	standupSprite[3].uvx = 0.0f;
	standupSprite[3].uvy = 0.0f;
	standupSprite[4].x = 0.0f;
	standupSprite[4].y = 1.0f;
	standupSprite[4].z = -STANDUP_SPRITE_HEIGHT;
	standupSprite[4].uvx = 0.0f;
	standupSprite[4].uvy = 1.0f;
	standupSprite[5].x = 1.0f;
	standupSprite[5].y = 1.0f;
	standupSprite[5].z = -STANDUP_SPRITE_HEIGHT;
	standupSprite[5].uvx = 1.0f;
	standupSprite[5].uvy = 1.0f;
}

// Hard code to 16 bytes
// 8x8 2 bit tiles
unsigned int MurMur32(struct gb_s* gb, unsigned int tileIndex, int hashSize)
{
	for (int i = 0; i < 8 * 2; ++i)
	{
		gb->vram[tileIndex + i];
	}
}
unsigned int hash10_bytes(struct gb_s* gb, unsigned int tileIndex, int lengthInts) {
	unsigned int* tile4Bytes = (unsigned int*)gb->vram;
	unsigned int h = 2166136261u;
	for (int i = 0; i < lengthInts; ++i)
	{
		h ^= tile4Bytes[tileIndex / sizeof(unsigned int) + i];
		h *= 16777619;
	}
	return h & 0x3ff;
}
#define tileHashEntries 4 * 1024
#define bytesToCompare 16
#define	DISPLAY_TILES_X 20
#define DISPLAY_TILES_Y 18
unsigned char hashMap[tileHashEntries];
unsigned int tileMap[DISPLAY_TILES_X * DISPLAY_TILES_Y];
unsigned char tileTypeMap[DISPLAY_TILES_X * DISPLAY_TILES_Y];
unsigned char tileModifierMap[DISPLAY_TILES_X * DISPLAY_TILES_Y];
int tileHeight[DISPLAY_TILES_X * DISPLAY_TILES_Y];

unsigned char fullTileData[tileHashEntries * 16];
unsigned char fullTileType[tileHashEntries];
unsigned char fullTileModifier[tileHashEntries];

enum TILE_TYPE
{
	TILE_TYPE_FLOOR = 0,
	TILE_TYPE_STAND_UP = 1,
	TILE_TYPE_WALL_NORTH = 3, // increment height
	TILE_TYPE_WALL_SOUTH = 4, // side wall do not increment height more than once
	TILE_TYPE_WALL_EAST = 5,
	TILE_TYPE_WALL_WEST = 6,
	TILE_TYPE_ROOF = 11,  // fall back to previous level after room
};

int GetTileType(struct gb_s* gb, int tileIndex, unsigned char* tileType, unsigned char* tileModifier)
{
	*tileType = 0;
	*tileModifier = 0;
	// TODO if found just fetch
	// if not add entry
	//unsigned int tileHash = hash10_bytes(gb, tileIndex, bytesToCompare / sizeof(unsigned int));
	//int tileType = hashMap[tileHash];
	for (int i = 0; i < numEntries; ++i)
	{
		bool bMatch = true;
		for (int j = 0; j < 16; ++j)
		{
			if (fullTileData[i * 16 + j] != gb->vram[tileIndex + j])
			{
				bMatch = false;
				break;
			}
		}
		if (bMatch == true)
		{
			*tileType = fullTileType[i];
			*tileModifier = fullTileModifier[i];
			return i;
		}
	}
	return -1;
}

#if !RP2
// PC EMU only
void SetTileType(struct gb_s* gb, int tileType)
{
	bool bWindowMode = gb->hram_io[IO_LCDC] & LCDC_WINDOW_ENABLE &&
		gb->hram_io[IO_LY] >= gb->display.WY &&
		gb->hram_io[IO_WX] <= 166;
	unsigned int tileIndex = GetTileIndex(
		gb,
		selectedTileX,
		selectedTileY,
		gb->hram_io[IO_SCX] / 8,
		gb->hram_io[IO_SCY] / 8,
		bWindowMode && selectedTileY * 8 >= gbMinWindowLYPerFrame);
	if (bWindowMode&& selectedTileY * 8 >= gbMinWindowLYPerFrame)
	{
		return;
	}
	//unsigned int tileHash = hash10_bytes(gb, tileIndex, bytesToCompare / sizeof(unsigned int));
	//hashMap[tileHash] = tileType;
	unsigned char tileTypeLast = 0;
	unsigned char tileModifierLast = 0;
	int index = GetTileType(gb, tileIndex, &tileTypeLast, &tileModifierLast);
	// Add entry
	if (index < 0)
	{
		for (int i = 0; i < 16; ++i)
		{
			fullTileData[numEntries * 16 + i] = gb->vram[tileIndex + i];
		}
		fullTileType[numEntries] = tileType;
		fullTileModifier[numEntries] = 0;
		numEntries++;
	}
	else if (tileType != tileTypeLast)
	{
		fullTileType[index] = tileType;
		fullTileModifier[index] = 0;
	}
	else if (tileType == tileTypeLast)
	{
		fullTileModifier[index]++;
		if (fullTileModifier[index] > 3)
		{
			fullTileModifier[index] = 0;
		}
	}
	SaveTileData("C:\\Peanut-GB\\LoZTileData.txt");
}
void MoveSelectedTile(int xDirection, int yDirection)
{
	selectedTileX = min(max(selectedTileX + xDirection, 0), DISPLAY_TILES_X - 1);
	selectedTileY = min(max(selectedTileY + yDirection, 0), DISPLAY_TILES_Y - 1);
}

void SaveTileData(char* tileDataPath)
{
	FILE* tileFile;
	errno_t err = fopen_s(&tileFile, tileDataPath, "wb");
	fwrite(&numEntries, sizeof(int), 1, tileFile);
	// Tile texture data
	fwrite(fullTileData, sizeof(unsigned char), sizeof(fullTileData) / sizeof(unsigned char), tileFile);
	// Tile data that determines how 3d tiles are drawn
	fwrite(fullTileType, sizeof(unsigned char), sizeof(fullTileType) / sizeof(unsigned char), tileFile);
	fwrite(fullTileModifier, sizeof(unsigned char), sizeof(fullTileModifier) / sizeof(unsigned char), tileFile);
	fclose(tileFile);
}

void LoadTileData(char* tileDataPath)
{
	FILE* tileFile;
	errno_t err = fopen_s(&tileFile, tileDataPath, "rb");
	if (err == 0 && tileFile)
	{
		fread(&numEntries, sizeof(int), 1, tileFile);
		// Tile texture data
		fread(fullTileData, sizeof(unsigned char), sizeof(fullTileData) / sizeof(unsigned char), tileFile);
		// Tile data that determines how 3d tiles are drawn
		fread(fullTileType, sizeof(unsigned char), sizeof(fullTileType) / sizeof(unsigned char), tileFile);
		fread(fullTileModifier, sizeof(unsigned char), sizeof(fullTileModifier) / sizeof(unsigned char), tileFile);
		fclose(tileFile);
	}
}
#endif

void clearSceneAndDepth(struct gb_s* gb, DrawMeshArgs* meshargs) {
	unsigned short bgColor = Color555To565(gb->direct.priv->selected_palette[BG_PALLET][0x0]);
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			meshargs->color[x * height + y] = bgColor;
			meshargs->depth[x * height + y] = 1000.0f;
		}
	}
}

void RenderFrame(struct gb_s* gb, unsigned short* colorBuffer, float* pos, float* rot)
{
	int playerX = gb->hram_io[0xFF98 - IO_ADDR];
	int playerY = gb->hram_io[0xFF99 - IO_ADDR];

	unsigned char messageBox = gb->wram[0xC19F - WRAM_0_ADDR];
	float defaultDepth = 20.0f;
	float roll = rot[0];
	float pitch = rot[1];
	float yaw = rot[2];
	float depth = defaultDepth + pos[2];

	float cameraFollowX = playerX / 8.0f - DISPLAY_TILES_X / 2.0f;
	float cameraFollowY = playerY / 8.0f - DISPLAY_TILES_Y / 2.0f;
	float zoomFactor = ((10.0f + 4.0f) / depth);
	cameraFollowX = min(max(cameraFollowX, -3 * zoomFactor), 2 * zoomFactor);
	cameraFollowY = min(max(cameraFollowY, -2 * zoomFactor), 2 * zoomFactor);

	bool bOrtho = false;
	bool bWindowMode = gb->hram_io[IO_LCDC] & LCDC_WINDOW_ENABLE &&
		gb->hram_io[IO_LY] >= gb->display.WY &&
		gb->hram_io[IO_WX] <= 166;

	bool bUpdateHeight = true;
	// During full screen menu or message return to top down ortho view
	if (messageBox != 0x0 ||
		gbMinWindowLYPerFrame < 128)
	{
		bUpdateHeight = false;
		bOrtho = false;
		roll = 0.0f;
		pitch = 0.0f;
		yaw = 0.0f;
		depth = defaultDepth;
		cameraFollowX = 0.0f;
		cameraFollowY = 0.0f;
	}
	float cameraOffsetX = -1.0f;
	float cameraOffsetY = -1.0f / 2.0f;
	vec3 playerpos = vec3C(cameraFollowX + cameraOffsetX, cameraFollowY + cameraOffsetY, depth);

	float angleOfView = 50.0f;
	float fov = 1.0f / tan(angleOfView * 0.5f * PI / 180.0f);
	float aspectratio = 1.0f;
	mat44 projmat = mat44C(
		vec4C(aspectratio * fov, 0.0f, 0.0f, 0.0f),
		vec4C(0.0f, fov, 0.0f, 0.0f),
		vec4C(0.0f, 0.0f, (far / (far - near)), ((-far * near) / (far - near))),
		vec4C(0.0f, 0.0f, 1.0f, 0.0f));
	if (bOrtho)
	{
		float left = -10.0f;
		float right = 10.0f;
		float top = 10.0f;
		float bottom = -10.0f;
		projmat = mat44C(
			vec4C(2.0f / (right - left), 0.0f, 0.0f, -(right + left) / (right - left)),
			vec4C(0.0f, 2.0f / (top - bottom), 0.0f, -(top + bottom) / (top - bottom)),
			vec4C(0.0f, 0.0f, 2.0f / (far - near), -(far * near) / (far - near)),
			vec4C(0.0f, 0.0f, 0.0f, 1.0f));
	}
	mat44 viewmat = mat44C(
		vec4C(1.0f, 0.0f, 0.0f, playerpos.x),
		vec4C(0.0f, 1.0f, 0.0f, playerpos.y),
		vec4C(0.0f, 0.0f, 1.0f, playerpos.z),
		vec4C(0.0f, 0.0f, 0.0f, 1.0f));

	mat44 rotmat = mat44C(
		vec4C(1.0f, 0.0f, 0.0, 0.0f),
		vec4C(0.0f, cosf(pitch), -sinf(pitch), 0.0f),
		vec4C(0.0f, sinf(pitch), cosf(pitch), 0.0f),
		vec4C(0.0f, 0.0f, 0.0f, 1.0f));
	mat44 rotmat1 = mat44C(
		vec4C(cosf(yaw), -sinf(yaw), 0.0f, 0.0f),
		vec4C(sinf(yaw), cosf(yaw), 0.0f, 0.0f),
		vec4C(0.0f, 0.0f, 1.0f, 0.0f),
		vec4C(0.0f, 0.0f, 0.0f, 1.0f));

	rotmat = matmatmul44(rotmat, rotmat1);
	viewmat = matmatmul44(viewmat, rotmat);

	DrawMeshArgs meshargs;
	GetMeshArgs(&meshargs, &playerpos, &projmat, &viewmat, &tileMesh);

	meshargs.color = colorBuffer;
	meshargs.gb = gb;

	clearSceneAndDepth(gb, &meshargs);

	// Position base align with scroll register values
	float positionBase[3] = { DISPLAY_TILES_X / 2.0f, DISPLAY_TILES_Y / 2.0f, 0.0f };
	float positionOffset[3] = { 0.0f, 0.0f, 0.0f };
	float meshpos[3] = { 0.0f, 0.0f, 0.0f };
	// Draw BG tiles
	meshargs.sampleBG = true;
	{
		for (int tileY = 0; tileY < DISPLAY_TILES_Y; ++tileY)
		{
			for (int tileX = 0; tileX < DISPLAY_TILES_X; ++tileX)
			{
				unsigned int tileIndex = GetTileIndex(
					gb,
					tileX,
					tileY,
					gb->hram_io[IO_SCX] / 8,
					gb->hram_io[IO_SCY] / 8,
					bWindowMode && tileY * 8 >= gbMinWindowLYPerFrame);
				int chacheIndex = tileY * DISPLAY_TILES_X + tileX;
				tileMap[chacheIndex] = tileIndex;
				unsigned char tileType = 0;
				unsigned char tileModifier = 0;
				int typeIndex = GetTileType(gb, tileIndex, &tileType, &tileModifier);
				tileTypeMap[chacheIndex] = tileType;
				tileModifierMap[chacheIndex] = tileModifier;
			}
		}
	}
	if (bUpdateHeight)
	{
		// Process map height and wall sides
		for (int tileXInd = 0; tileXInd < DISPLAY_TILES_X; ++tileXInd)
		{
			bool bHasLastFloorHeight = false;
			char lastFloorHeight = 0;
			int currentHeight = 0;
			int backGroundStart = 2;
			for (int tileYInd = backGroundStart; tileYInd < DISPLAY_TILES_Y; ++tileYInd)
			{
				int tileYFlip = DISPLAY_TILES_Y - 1 - tileYInd;
				int cacheIndex = tileYFlip * DISPLAY_TILES_X + tileXInd;
				int cacheIndexLeft = cacheIndex - 1;
				int cacheIndexRight = cacheIndex + 1;
				int cacheIndexDown = (tileYFlip + 1) * DISPLAY_TILES_X + tileXInd;
				tileHeight[cacheIndex] = currentHeight;
				unsigned char tileType = tileTypeMap[cacheIndex];
				// Check for modified by neightbor to left and right
				if (tileType == TILE_TYPE_WALL_NORTH)
				{
					currentHeight += LEVEL_HEIGHT;
				}
				else if (tileType == TILE_TYPE_WALL_SOUTH)
				{
					currentHeight -= LEVEL_HEIGHT;
					tileHeight[cacheIndex] = currentHeight;
				}
				else if (tileType == TILE_TYPE_FLOOR)
				{
					// Special wall modifiers to flatten door openings that share tiles with UI and grass
					// This lowers floors one tile to the left or right of a south wall
					if (tileTypeMap[cacheIndexLeft] == TILE_TYPE_WALL_SOUTH &&
						tileModifierMap[cacheIndexLeft] == 1 &&
						tileXInd > 0) // check left
					{
						currentHeight -= LEVEL_HEIGHT;
					}
					else if (tileTypeMap[cacheIndexRight] == TILE_TYPE_WALL_SOUTH &&
						tileModifierMap[cacheIndexRight] == 2 &&
						tileXInd < DISPLAY_TILES_X - 1) // check right
					{
						currentHeight -= LEVEL_HEIGHT;
					}
				}
				else if (tileType == TILE_TYPE_STAND_UP)
				{
					// Keep same height but modify position in draw loop
				}
				// Reset floor height behind roof
				if (tileType != TILE_TYPE_ROOF)
				{
					if (tileTypeMap[cacheIndexDown] == TILE_TYPE_ROOF &&
						tileYInd >= backGroundStart)
					{
						currentHeight -= tileModifierMap[cacheIndexDown] * LEVEL_HEIGHT;
						tileHeight[cacheIndex] = currentHeight;
					}
				}
			}
		}
		// Horizontal pass
		// Connect floors and roofs that have changing tile height
	}
	else
	{
		for (int tileXInd = 0; tileXInd < DISPLAY_TILES_X; ++tileXInd)
		{
			for (int tileYInd = 0; tileYInd < DISPLAY_TILES_Y; ++tileYInd)
			{
				int cacheIndex = tileYInd * DISPLAY_TILES_X + tileXInd;
				tileHeight[cacheIndex] = 0.0f;
			}
		}
	}

	{
		meshargs.shaderstate.MeshColor = 0x001f;
		for (int tileY = 0; tileY < DISPLAY_TILES_Y; ++tileY)
		{
			for (int tileX = 0; tileX < DISPLAY_TILES_X; ++tileX)
			{
				meshargs.blendstate.AlphaClipEnable = false;
				if (tileX == selectedTileX && tileY == selectedTileY)
				{
					meshargs.shaderstate.UseMeshColor = true;
				}
				else
				{
					meshargs.shaderstate.UseMeshColor = false;
				}
				int cacheIndex = tileY * DISPLAY_TILES_X + tileX;
				meshargs.tileX = tileX;
				meshargs.tileY = tileY;
				meshargs.tileIndex = tileMap[cacheIndex];
				unsigned char tileType = tileTypeMap[cacheIndex];
				positionOffset[2] = -1.0f * tileHeight[cacheIndex];
				if (!bUpdateHeight)
				{
					meshargs.vertices = verts;
				}
				else if (tileType == TILE_TYPE_WALL_NORTH)
				{
					meshargs.vertices = rampUp;
				}
				else if (tileType == TILE_TYPE_WALL_SOUTH)
				{
					meshargs.vertices = rampDown;
				}
				else if (tileType == TILE_TYPE_WALL_EAST)
				{
					meshargs.vertices = rampRight;
				}
				else if (tileType == TILE_TYPE_WALL_WEST)
				{
					meshargs.vertices = rampLeft;
				}
				else if (tileType == TILE_TYPE_STAND_UP)
				{
					positionOffset[2] -= tileModifierMap[cacheIndex] * STANDUP_SPRITE_HEIGHT;
					meshargs.vertices = standupSprite;
					meshargs.blendstate.AlphaClipEnable = true;
				}
				else // floor and roof
				{
					meshargs.vertices = verts;
				}
				meshpos[0] = positionOffset[0] + positionBase[0];
				meshpos[1] = positionOffset[1] + positionBase[1];
				meshpos[2] = positionOffset[2] + positionBase[2];
				rastermeshoffset(&meshargs, meshpos);
				positionOffset[0] -= 1.0f;
			}
			positionOffset[0] = 0.0f;
			positionOffset[1] -= 1.0f;
		}
	}

	float minX = 0.0f + positionBase[0];
	float maxX = -1.0f * (DISPLAY_TILES_X - 1) + positionBase[0];
	float minY = 0.0f + positionBase[1];
	float maxY = -1.0f * (DISPLAY_TILES_Y - 1) + positionBase[1];
	// Draw objs
	positionOffset[0] = 0.0f;
	positionOffset[1] = 0.0f;
	positionOffset[2] = 0.0f;
	uint8_t sprite_number;
	meshargs.sampleBG = false;
	meshargs.vertices = spriteVerts;
	for (sprite_number = NUM_SPRITES - 1;
		sprite_number != 0xFF;
		sprite_number--)
	{
		float xPosition = 0.0f;
		float yPosition = 0.0f;
		int tileIndex = 0;
		uint8_t attributes = 0;

		GetSpriteProperties(
			gb,
			sprite_number,
			&xPosition,
			&yPosition,
			&tileIndex,
			&attributes);
		static int adjx = 12;
		static int adjy = 12;
		// Unused sprites are hidden on the top margin when not active
		if (yPosition > 0.0f)
		{
			float spriteDepthBias = 1.0f;
			positionOffset[0] = (xPosition) / 8.0f + 8 / 8;
			positionOffset[1] = (yPosition) / 8.0f + 16 / 8;
			meshargs.tileIndex = tileIndex;
			meshargs.attributes = attributes;
			meshpos[0] = adjx - positionOffset[0];
			meshpos[1] = adjy - positionOffset[1];
			int tileMapX = DISPLAY_TILES_X * ((maxX - meshpos[0]) / (maxX - minX));
			int tileMapY = DISPLAY_TILES_Y * ((maxY - meshpos[1]) / (maxY - minY));
			tileMapX = clampi(roundf(DISPLAY_TILES_X - 1 - tileMapX - 0.5f), 0, DISPLAY_TILES_X - 1);
			tileMapY = clampi(DISPLAY_TILES_Y - 1 - tileMapY, 0, DISPLAY_TILES_Y - 1);
			int cacheIndex = tileMapY * DISPLAY_TILES_X + tileMapX;
			meshpos[2] = -1.0f * tileHeight[cacheIndex] - spriteDepthBias;
			rastermeshoffset(&meshargs, meshpos);
		}
	}
}

#endif
