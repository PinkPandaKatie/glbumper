#include "graphics.h"

class GLState : public GCList {
    static GCList root;

protected:
    GLState() {
        moveto(&root);
    }
    virtual ~GLState() {
        remove();
    }
public:
    virtual void dump(ostream& o) { };
    static void dumpall(ostream& o) {
        for (GCList* l = root.next; l != &root; l = l->next) {
            static_cast<GLState*>(l)->dump(o);
        }
    }
};
GCList GLState::root;


class GLEnableState : public GLState {
    string name;
    GLint val;
    
public:
    GLEnableState(GLint val, string name) : name(name), val(val) { }
    virtual void dump(ostream& o) {
        bool en = glIsEnabled(val);
        o << "isEnabled(" << name << ") -> ";
        if (en) {
            o << "TRUE";
        } else {
            o << "FALSE";
        }
        o << endl;
    }
};

class GLIntegerState : public GLState {
    string name;
    GLint val;

    int n;
public:
    GLIntegerState(GLint val, string name, int n = 1) : name(name), val(val), n(n) { }
    virtual void dump(ostream& o) {
        GLint p[n];
        glGetIntegerv(val, p);

        o << "glGetInteger(" << name << ") -> [";
        bool first = true;
        for (int i = 0; i < n; i++) {
            if (!first) {
                o << ", ";
            }
            first = false;
            o << p[i];
        }
        o << "]"<<endl;

    }
};

class GLDoubleState : public GLState {
    string name;
    GLint val;

    int n;
public:
    GLDoubleState(GLint val, string name, int n = 1) : name(name), val(val), n(n) { }
    virtual void dump(ostream& o) {
        GLdouble p[n];
        glGetDoublev(val, p);

        o << "glGetDouble(" << name << ") -> [";
        bool first = true;
        for (int i = 0; i < n; i++) {
            if (!first) {
                o << ", ";
            }
            first = false;
            o << p[i];
        }
        o << "]"<<endl;

    }
};


GLEnableState enable_alpha_test (GL_ALPHA_TEST, "GL_ALPHA_TEST");
GLEnableState enable_auto_normal (GL_AUTO_NORMAL, "GL_AUTO_NORMAL");
GLEnableState enable_blend (GL_BLEND, "GL_BLEND");
GLEnableState enable_clip_plane0 (GL_CLIP_PLANE0, "GL_CLIP_PLANE0");
GLEnableState enable_clip_plane1 (GL_CLIP_PLANE1, "GL_CLIP_PLANE1");
GLEnableState enable_clip_plane2 (GL_CLIP_PLANE2, "GL_CLIP_PLANE2");
GLEnableState enable_clip_plane3 (GL_CLIP_PLANE3, "GL_CLIP_PLANE3");
GLEnableState enable_clip_plane4 (GL_CLIP_PLANE4, "GL_CLIP_PLANE4");
GLEnableState enable_clip_plane5 (GL_CLIP_PLANE5, "GL_CLIP_PLANE5");
GLEnableState enable_color_array (GL_COLOR_ARRAY, "GL_COLOR_ARRAY");
GLEnableState enable_color_logic_op (GL_COLOR_LOGIC_OP, "GL_COLOR_LOGIC_OP");
GLEnableState enable_color_material (GL_COLOR_MATERIAL, "GL_COLOR_MATERIAL");
GLEnableState enable_color_table (GL_COLOR_TABLE, "GL_COLOR_TABLE");
GLEnableState enable_convolution_1d (GL_CONVOLUTION_1D, "GL_CONVOLUTION_1D");
GLEnableState enable_convolution_2d (GL_CONVOLUTION_2D, "GL_CONVOLUTION_2D");
GLEnableState enable_cull_face (GL_CULL_FACE, "GL_CULL_FACE");
GLEnableState enable_depth_test (GL_DEPTH_TEST, "GL_DEPTH_TEST");
GLEnableState enable_dither (GL_DITHER, "GL_DITHER");
GLEnableState enable_edge_flag_array (GL_EDGE_FLAG_ARRAY, "GL_EDGE_FLAG_ARRAY");
GLEnableState enable_fog (GL_FOG, "GL_FOG");
GLEnableState enable_histogram (GL_HISTOGRAM, "GL_HISTOGRAM");
GLEnableState enable_index_array (GL_INDEX_ARRAY, "GL_INDEX_ARRAY");
GLEnableState enable_index_logic_op (GL_INDEX_LOGIC_OP, "GL_INDEX_LOGIC_OP");
GLEnableState enable_light0 (GL_LIGHT0, "GL_LIGHT0");
GLEnableState enable_light1 (GL_LIGHT1, "GL_LIGHT0");
GLEnableState enable_light2 (GL_LIGHT2, "GL_LIGHT0");
GLEnableState enable_light3 (GL_LIGHT3, "GL_LIGHT0");
GLEnableState enable_light4 (GL_LIGHT4, "GL_LIGHT0");
GLEnableState enable_light5 (GL_LIGHT5, "GL_LIGHT0");
GLEnableState enable_light6 (GL_LIGHT6, "GL_LIGHT0");
GLEnableState enable_light7 (GL_LIGHT7, "GL_LIGHT0");
GLEnableState enable_lighting (GL_LIGHTING, "GL_LIGHTING");
GLEnableState enable_line_smooth (GL_LINE_SMOOTH, "GL_LINE_SMOOTH");
GLEnableState enable_line_stipple (GL_LINE_STIPPLE, "GL_LINE_STIPPLE");
GLEnableState enable_map1_color_4 (GL_MAP1_COLOR_4, "GL_MAP1_COLOR_4");
GLEnableState enable_map1_index (GL_MAP1_INDEX, "GL_MAP1_INDEX");
GLEnableState enable_map1_normal (GL_MAP1_NORMAL, "GL_MAP1_NORMAL");
GLEnableState enable_map1_texture_coord_1 (GL_MAP1_TEXTURE_COORD_1, "GL_MAP1_TEXTURE_COORD_1");
GLEnableState enable_map1_texture_coord_2 (GL_MAP1_TEXTURE_COORD_2, "GL_MAP1_TEXTURE_COORD_2");
GLEnableState enable_map1_texture_coord_3 (GL_MAP1_TEXTURE_COORD_3, "GL_MAP1_TEXTURE_COORD_3");
GLEnableState enable_map1_texture_coord_4 (GL_MAP1_TEXTURE_COORD_4, "GL_MAP1_TEXTURE_COORD_4");
GLEnableState enable_map2_color_4 (GL_MAP2_COLOR_4, "GL_MAP2_COLOR_4");
GLEnableState enable_map2_index (GL_MAP2_INDEX, "GL_MAP2_INDEX");
GLEnableState enable_map2_normal (GL_MAP2_NORMAL, "GL_MAP2_NORMAL");
GLEnableState enable_map2_texture_coord_1 (GL_MAP2_TEXTURE_COORD_1, "GL_MAP2_TEXTURE_COORD_1");
GLEnableState enable_map2_texture_coord_2 (GL_MAP2_TEXTURE_COORD_2, "GL_MAP2_TEXTURE_COORD_2");
GLEnableState enable_map2_texture_coord_3 (GL_MAP2_TEXTURE_COORD_3, "GL_MAP2_TEXTURE_COORD_3");
GLEnableState enable_map2_texture_coord_4 (GL_MAP2_TEXTURE_COORD_4, "GL_MAP2_TEXTURE_COORD_4");
GLEnableState enable_map2_vertex_3 (GL_MAP2_VERTEX_3, "GL_MAP2_VERTEX_3");
GLEnableState enable_map2_vertex_4 (GL_MAP2_VERTEX_4, "GL_MAP2_VERTEX_4");
GLEnableState enable_minmax (GL_MINMAX, "GL_MINMAX");
GLEnableState enable_normal_array (GL_NORMAL_ARRAY, "GL_NORMAL_ARRAY");
GLEnableState enable_normalize (GL_NORMALIZE, "GL_NORMALIZE");
GLEnableState enable_point_smooth (GL_POINT_SMOOTH, "GL_POINT_SMOOTH");
GLEnableState enable_polygon_smooth (GL_POLYGON_SMOOTH, "GL_POLYGON_SMOOTH");
GLEnableState enable_polygon_offset_fill (GL_POLYGON_OFFSET_FILL, "GL_POLYGON_OFFSET_FILL");
GLEnableState enable_polygon_offset_line (GL_POLYGON_OFFSET_LINE, "GL_POLYGON_OFFSET_LINE");
GLEnableState enable_polygon_offset_point (GL_POLYGON_OFFSET_POINT, "GL_POLYGON_OFFSET_POINT");
GLEnableState enable_polygon_stipple (GL_POLYGON_STIPPLE, "GL_POLYGON_STIPPLE");
GLEnableState enable_post_color_matrix_color_table (GL_POST_COLOR_MATRIX_COLOR_TABLE, "GL_POST_COLOR_MATRIX_COLOR_TABLE");
GLEnableState enable_post_convolution_color_table (GL_POST_CONVOLUTION_COLOR_TABLE, "GL_POST_CONVOLUTION_COLOR_TABLE");
GLEnableState enable_rescale_normal (GL_RESCALE_NORMAL, "GL_RESCALE_NORMAL");
GLEnableState enable_scissor_test (GL_SCISSOR_TEST, "GL_SCISSOR_TEST");
GLEnableState enable_separable_2d (GL_SEPARABLE_2D, "GL_SEPARABLE_2D");
GLEnableState enable_stencil_test (GL_STENCIL_TEST, "GL_STENCIL_TEST");
GLEnableState enable_texture_1d (GL_TEXTURE_1D, "GL_TEXTURE_1D");
GLEnableState enable_texture_2d (GL_TEXTURE_2D, "GL_TEXTURE_2D");
GLEnableState enable_texture_3d (GL_TEXTURE_3D, "GL_TEXTURE_3D");
GLEnableState enable_texture_coord_array (GL_TEXTURE_COORD_ARRAY, "GL_TEXTURE_COORD_ARRAY");
GLEnableState enable_texture_gen_q (GL_TEXTURE_GEN_Q, "GL_TEXTURE_GEN_Q");
GLEnableState enable_texture_gen_r (GL_TEXTURE_GEN_R, "GL_TEXTURE_GEN_R");
GLEnableState enable_texture_gen_s (GL_TEXTURE_GEN_S, "GL_TEXTURE_GEN_S");
GLEnableState enable_texture_gen_t (GL_TEXTURE_GEN_T, "GL_TEXTURE_GEN_T");
GLEnableState enable_vertex_array (GL_VERTEX_ARRAY, "GL_VERTEX_ARRAY");


// params returns one value, the color index used to clear the color index
// buffers. The initial value is 0. See glClearIndex.
GLIntegerState st_index_clear_value(GL_INDEX_CLEAR_VALUE, "GL_INDEX_CLEAR_VALUE", 1);

// params returns a single boolean value indicating whether stencil testing of
// fragments is enabled. The initial value is GL_FALSE. See glStencilFunc and glStencilOp.
GLIntegerState st_stencil_test(GL_STENCIL_TEST, "GL_STENCIL_TEST", 1);

// params returns one value, the maximum supported size of a glPixelMap lookup
// table. The value must be at least 32. See glPixelMap.
GLIntegerState st_max_pixel_map_table(GL_MAX_PIXEL_MAP_TABLE, "GL_MAX_PIXEL_MAP_TABLE", 1);

// params returns one value, the row length used for writing pixel data to memory.
// The initial value is 0. See glPixelStore.
GLIntegerState st_pack_row_length(GL_PACK_ROW_LENGTH, "GL_PACK_ROW_LENGTH", 1);

// params returns a single boolean value indicating whether specular reflection
// calculations treat the viewer as being local to the scene. The initial value is GL_FALSE.
// See glLightModel.
GLIntegerState st_light_model_local_viewer(GL_LIGHT_MODEL_LOCAL_VIEWER, "GL_LIGHT_MODEL_LOCAL_VIEWER", 1);

// params returns a single boolean value indicating whether one or more material
// parameters are tracking the current color. The initial value is GL_FALSE. See
// glColorMaterial.
GLIntegerState st_color_material(GL_COLOR_MATERIAL, "GL_COLOR_MATERIAL", 1);

// params returns four values: the x, y, z, and w components of the current raster
// position. x, y, and z are in window coordinates, and w is in clip coordinates. The
// initial value is (0, 0, 0, 1). See glRasterPos.
GLIntegerState st_current_raster_position(GL_CURRENT_RASTER_POSITION, "GL_CURRENT_RASTER_POSITION", 4);

// params returns one value, the alpha scale factor applied to RGBA fragments
// after color matrix transformations. The initial value is 1. See glPixelTransfer.
GLDoubleState st_post_color_matrix_alpha_scale(GL_POST_COLOR_MATRIX_ALPHA_SCALE, "GL_POST_COLOR_MATRIX_ALPHA_SCALE", 1);

// params returns one value, the number of names on the selection name stack. The
// initial value is 0. See glPushName.
GLIntegerState st_name_stack_depth(GL_NAME_STACK_DEPTH, "GL_NAME_STACK_DEPTH", 1);

// params returns two values, the smallest and largest supported widths for
// antialiased points. See glPointSize.
GLIntegerState st_smooth_point_size_range(GL_SMOOTH_POINT_SIZE_RANGE, "GL_SMOOTH_POINT_SIZE_RANGE", 2);

// params returns a single boolean value indicating whether antialiasing of points
// is enabled. The initial value is GL_FALSE. See glPointSize.
GLIntegerState st_point_smooth(GL_POINT_SMOOTH, "GL_POINT_SMOOTH", 1);

// params returns four values: the red, green, blue, and alpha values used to
// clear the accumulation buffer. Integer values, if requested, are linearly mapped from
// the internal floating-point representation such that 1.0 returns the most positive
// representable integer value, and -1.0 returns the most negative representable integer value.
// The initial value is (0, 0, 0, 0). See glClearAccum.
GLDoubleState st_accum_clear_value(GL_ACCUM_CLEAR_VALUE, "GL_ACCUM_CLEAR_VALUE", 4);

// params returns two values: the smallest and largest supported sizes for
// antialiased points. The smallest size must be at most 1, and the largest size must be at
// least 1. See glPointSize.
GLDoubleState st_point_size_range(GL_POINT_SIZE_RANGE, "GL_POINT_SIZE_RANGE", 2);

// params returns one value, the value that is used to clear the depth buffer.
// Integer values, if requested, are linearly mapped from the internal floating-point
// representation such that 1.0 returns the most positive representable integer value, and -1.0
// returns the most negative representable integer value. The initial value is 1. See
// glClearDepth.
GLDoubleState st_depth_clear_value(GL_DEPTH_CLEAR_VALUE, "GL_DEPTH_CLEAR_VALUE", 1);

// params returns a single boolean value indicating whether dithering of fragment
// colors and indices is enabled. The initial value is GL_TRUE.
GLIntegerState st_dither(GL_DITHER, "GL_DITHER", 1);

// params returns one value, the number of matrices on the texture matrix stack.
// The initial value is 1. See glPushMatrix.
GLIntegerState st_texture_stack_depth(GL_TEXTURE_STACK_DEPTH, "GL_TEXTURE_STACK_DEPTH", 1);

// params returns one value, the size difference between adjacent supported sizes
// for antialiased points. See glPointSize.
GLIntegerState st_point_size_granularity(GL_POINT_SIZE_GRANULARITY, "GL_POINT_SIZE_GRANULARITY", 1);

// params returns a single boolean value indicating whether post convolution
// lookup is enabled. The initial value is GL_FALSE. See glColorTable.
GLIntegerState st_post_convolution_color_table(GL_POST_CONVOLUTION_COLOR_TABLE, "GL_POST_CONVOLUTION_COLOR_TABLE", 1);

// params returns a single boolean value indicating whether the GL is in RGBA mode
// (true) or color index mode (false). See glColor.
GLIntegerState st_rgba_mode(GL_RGBA_MODE, "GL_RGBA_MODE", 1);

// params returns a single boolean value indicating whether antialiasing of lines
// is enabled. The initial value is GL_FALSE. See glLineWidth.
GLIntegerState st_line_smooth(GL_LINE_SMOOTH, "GL_LINE_SMOOTH", 1);

// params returns one value, the data type of each coordinate in the normal array.
// The initial value is GL_FLOAT. See glNormalPointer.
GLIntegerState st_normal_array_type(GL_NORMAL_ARRAY_TYPE, "GL_NORMAL_ARRAY_TYPE", 1);

// params returns one value, the alpha bias factor used during pixel transfers.
// The initial value is 0. See glPixelTransfer.
GLDoubleState st_alpha_bias(GL_ALPHA_BIAS, "GL_ALPHA_BIAS", 1);

// params returns one value, the data type of each coordinate in the vertex array.
// The initial value is GL_FLOAT. See glVertexPointer.
GLIntegerState st_vertex_array_type(GL_VERTEX_ARRAY_TYPE, "GL_VERTEX_ARRAY_TYPE", 1);

// params returns one value, the number of coordinates per vertex in the vertex
// array. The initial value is 4. See glVertexPointer.
GLIntegerState st_vertex_array_size(GL_VERTEX_ARRAY_SIZE, "GL_VERTEX_ARRAY_SIZE", 1);

// params returns a single boolean value indicating whether scissoring is enabled.
// The initial value is GL_FALSE. See glScissor.
GLIntegerState st_scissor_test(GL_SCISSOR_TEST, "GL_SCISSOR_TEST", 1);

// params returns one value, the reference value that is compared with the
// contents of the stencil buffer. The initial value is 0. See glStencilFunc.
GLIntegerState st_stencil_ref(GL_STENCIL_REF, "GL_STENCIL_REF", 1);

// params returns one value, the symbolic name of the alpha test function. The
// initial value is GL_ALWAYS. See glAlphaFunc.
GLIntegerState st_alpha_test_func(GL_ALPHA_TEST_FUNC, "GL_ALPHA_TEST_FUNC", 1);

// params returns one value, the amount that color and stencil indices are shifted
// during pixel transfers. The initial value is 0. See glPixelTransfer.
GLIntegerState st_index_shift(GL_INDEX_SHIFT, "GL_INDEX_SHIFT", 1);

// params returns one value, the line stipple repeat factor. The initial value is
// 1. See glLineStipple.
GLIntegerState st_line_stipple_repeat(GL_LINE_STIPPLE_REPEAT, "GL_LINE_STIPPLE_REPEAT", 1);

// params returns one value, a rough estimate of the largest 3D texture that the
// GL can handle. If the GL version is 1.2 or greater, use GL_PROXY_TEXTURE_3D to
// determine if a texture is too large. See glTexImage3D.
GLIntegerState st_max_3d_texture_size(GL_MAX_3D_TEXTURE_SIZE, "GL_MAX_3D_TEXTURE_SIZE", 1);

// params returns a single boolean value indicating whether 1D convolution is
// enabled. The initial value is GL_FALSE. See glConvolutionFilter1D.
GLIntegerState st_convolution_1d(GL_CONVOLUTION_1D, "GL_CONVOLUTION_1D", 1);

// params returns a single boolean value indicating whether blending is enabled.
// The initial value is GL_FALSE. See glBlendFunc.
GLIntegerState st_blend(GL_BLEND, "GL_BLEND", 1);

// params returns one value, the type of the feedback buffer. See
// glFeedbackBuffer.
GLIntegerState st_feedback_buffer_type(GL_FEEDBACK_BUFFER_TYPE, "GL_FEEDBACK_BUFFER_TYPE", 1);

// params returns a single boolean value indicating whether 1D evaluation
// generates 3D texture coordinates. The initial value is GL_FALSE. See glMap1.
GLIntegerState st_map1_texture_coord_3(GL_MAP1_TEXTURE_COORD_3, "GL_MAP1_TEXTURE_COORD_3", 1);

// params returns a single boolean value indicating whether 1D evaluation
// generates 2D texture coordinates. The initial value is GL_FALSE. See glMap1.
GLIntegerState st_map1_texture_coord_2(GL_MAP1_TEXTURE_COORD_2, "GL_MAP1_TEXTURE_COORD_2", 1);

// params returns a single boolean value indicating whether 1D evaluation
// generates 1D texture coordinates. The initial value is GL_FALSE. See glMap1.
GLIntegerState st_map1_texture_coord_1(GL_MAP1_TEXTURE_COORD_1, "GL_MAP1_TEXTURE_COORD_1", 1);

// params returns a single boolean value indicating whether stereo buffers (left
// and right) are supported.
GLIntegerState st_stereo(GL_STEREO, "GL_STEREO", 1);

// params returns a single boolean value indicating whether 1D evaluation
// generates 4D texture coordinates. The initial value is GL_FALSE. See glMap1.
GLIntegerState st_map1_texture_coord_4(GL_MAP1_TEXTURE_COORD_4, "GL_MAP1_TEXTURE_COORD_4", 1);

// params returns single enumerated value indicating whether specular reflection
// calculations are separated from normal lighting computations. The initial value is
// GL_SINGLE_COLOR.
GLDoubleState st_light_model_color_control(GL_LIGHT_MODEL_COLOR_CONTROL, "GL_LIGHT_MODEL_COLOR_CONTROL", 1);

// params returns one value, the red scale factor applied to RGBA fragments after
// color matrix transformations. The initial value is 1. See glPixelTransfer.
GLDoubleState st_post_color_matrix_red_scale(GL_POST_COLOR_MATRIX_RED_SCALE, "GL_POST_COLOR_MATRIX_RED_SCALE", 1);

// params returns one value, the current color index. The initial value is 1. See
// glIndex.
GLIntegerState st_current_index(GL_CURRENT_INDEX, "GL_CURRENT_INDEX", 1);

// params returns a single boolean value indicating whether 1D evaluation
// generates color indices. The initial value is GL_FALSE. See glMap1.
GLIntegerState st_map1_index(GL_MAP1_INDEX, "GL_MAP1_INDEX", 1);

// params returns one value, the width difference between adjacent supported
// widths for antialiased lines. See glLineWidth.
GLIntegerState st_line_width_granularity(GL_LINE_WIDTH_GRANULARITY, "GL_LINE_WIDTH_GRANULARITY", 1);

// params returns one value, the maximum supported depth of the texture matrix
// stack. The value must be at least 2. See glPushMatrix.
GLIntegerState st_max_texture_stack_depth(GL_MAX_TEXTURE_STACK_DEPTH, "GL_MAX_TEXTURE_STACK_DEPTH", 1);

// params returns a single boolean value indicating whether automatic generation
// of the S texture coordinate is enabled. The initial value is GL_FALSE. See
// glTexGen.
GLIntegerState st_texture_gen_s(GL_TEXTURE_GEN_S, "GL_TEXTURE_GEN_S", 1);

// params returns one value, the number of alpha bitplanes in the accumulation
// buffer.
GLIntegerState st_accum_alpha_bits(GL_ACCUM_ALPHA_BITS, "GL_ACCUM_ALPHA_BITS", 1);

// params returns one value, the depth scale factor used during pixel transfers.
// The initial value is 1. See glPixelTransfer.
GLDoubleState st_depth_scale(GL_DEPTH_SCALE, "GL_DEPTH_SCALE", 1);

// params returns a single boolean value indicating whether automatic generation
// of the r texture coordinate is enabled. The initial value is GL_FALSE. See
// glTexGen.
GLIntegerState st_texture_gen_r(GL_TEXTURE_GEN_R, "GL_TEXTURE_GEN_R", 1);

// params returns one value, the blue bias factor applied to RGBA fragments after
// convolution. The initial value is 0. See glPixelTransfer.
GLDoubleState st_post_convolution_blue_bias(GL_POST_CONVOLUTION_BLUE_BIAS, "GL_POST_CONVOLUTION_BLUE_BIAS", 1);

// params returns a single boolean value indicating whether lighting is enabled.
// The initial value is GL_FALSE. See glLightModel.
GLIntegerState st_lighting(GL_LIGHTING, "GL_LIGHTING", 1);

// params returns one value, a symbolic constant indicating what action is taken
// when the stencil test passes and the depth test passes. The initial value is
// GL_KEEP. See glStencilOp.
GLIntegerState st_stencil_pass_depth_pass(GL_STENCIL_PASS_DEPTH_PASS, "GL_STENCIL_PASS_DEPTH_PASS", 1);

// params returns one value, the byte offset between consecutive elements in the
// texture coordinate array. The initial value is 0. See glTexCoordPointer.
GLIntegerState st_texture_coord_array_stride(GL_TEXTURE_COORD_ARRAY_STRIDE, "GL_TEXTURE_COORD_ARRAY_STRIDE", 1);

// params returns one value, a symbolic constant indicating the construction mode
// of the display list currently under construction. The initial value is 0. See
// glNewList.
GLIntegerState st_list_mode(GL_LIST_MODE, "GL_LIST_MODE", 1);

// params returns one value, the distance from the eye to the current raster
// position. The initial value is 0. See glRasterPos.
GLDoubleState st_current_raster_distance(GL_CURRENT_RASTER_DISTANCE, "GL_CURRENT_RASTER_DISTANCE", 1);

// params returns one value, the maximum recursion depth allowed during
// display-list traversal. The value must be at least 64. See glCallList.
GLIntegerState st_max_list_nesting(GL_MAX_LIST_NESTING, "GL_MAX_LIST_NESTING", 1);

// params returns one value, the red scale factor applied to RGBA fragments after
// convolution. The initial value is 1. See glPixelTransfer.
GLDoubleState st_post_convolution_red_scale(GL_POST_CONVOLUTION_RED_SCALE, "GL_POST_CONVOLUTION_RED_SCALE", 1);

// params returns one value, the alpha scale factor used during pixel transfers.
// The initial value is 1. See glPixelTransfer.
GLDoubleState st_alpha_scale(GL_ALPHA_SCALE, "GL_ALPHA_SCALE", 1);

// params returns a single boolean value indicating whether separate materials are
// used to compute lighting for front- and back-facing polygons. The initial value is
// GL_FALSE. See glLightModel.
GLIntegerState st_light_model_two_side(GL_LIGHT_MODEL_TWO_SIDE, "GL_LIGHT_MODEL_TWO_SIDE", 1);

// params returns one value, a symbolic constant indicating which color buffer is
// selected for reading. The initial value is GL_BACK if there is a back buffer, otherwise
// it is GL_FRONT. See glReadPixels and glAccum.
GLIntegerState st_read_buffer(GL_READ_BUFFER, "GL_READ_BUFFER", 1);

// params returns one value, the number of pixel locations skipped before the
// first pixel is written into memory. The initial value is 0. See glPixelStore.
GLIntegerState st_pack_skip_pixels(GL_PACK_SKIP_PIXELS, "GL_PACK_SKIP_PIXELS", 1);

// params returns one value, a symbolic constant indicating whether the blend
// equation is GL_FUNC_ADD, GL_MIN or GL_MAX. See glBlendEquation.
GLIntegerState st_blend_equation(GL_BLEND_EQUATION, "GL_BLEND_EQUATION", 1);

// params returns one value, the number of coordinates per element in the texture
// coordinate array. The initial value is 4. See glTexCoordPointer.
GLIntegerState st_texture_coord_array_size(GL_TEXTURE_COORD_ARRAY_SIZE, "GL_TEXTURE_COORD_ARRAY_SIZE", 1);

// params returns one value, the number of blue bitplanes in the accumulation
// buffer.
GLIntegerState st_accum_blue_bits(GL_ACCUM_BLUE_BITS, "GL_ACCUM_BLUE_BITS", 1);

// params returns a single boolean value indicating whether antialiasing of
// polygons is enabled. The initial value is GL_FALSE. See glPolygonMode.
GLIntegerState st_polygon_smooth(GL_POLYGON_SMOOTH, "GL_POLYGON_SMOOTH", 1);

// params returns one value, a symbolic constant indicating what function is used
// to compare the stencil reference value with the stencil buffer value. The initial
// value is GL_ALWAYS. See glStencilFunc.
GLIntegerState st_stencil_func(GL_STENCIL_FUNC, "GL_STENCIL_FUNC", 1);

// params returns three values: the x, y, and z values of the current normal.
// Integer values, if requested, are linearly mapped from the internal floating-point
// representation such that 1.0 returns the most positive representable integer value, and -1.0
// returns the most negative representable integer value. The initial value is (0, 0, 1).
// See glNormal.
GLDoubleState st_current_normal(GL_CURRENT_NORMAL, "GL_CURRENT_NORMAL", 3);

// params returns a single boolean value indicating whether the color array is
// enabled. The initial value is GL_FALSE. See glColorPointer.
GLIntegerState st_color_array(GL_COLOR_ARRAY, "GL_COLOR_ARRAY", 1);

// params returns a single boolean value indicating whether 2D convolution is
// enabled. The initial value is GL_FALSE. See glConvolutionFilter2D.
GLIntegerState st_convolution_2d(GL_CONVOLUTION_2D, "GL_CONVOLUTION_2D", 1);

// params returns one value, the number of matrices on the modelview matrix stack.
// The initial value is 1. See glPushMatrix.
GLIntegerState st_modelview_stack_depth(GL_MODELVIEW_STACK_DEPTH, "GL_MODELVIEW_STACK_DEPTH", 1);

// params returns one value, a symbolic constant indicating which material
// parameters are tracking the current color. The initial value is GL_AMBIENT_AND_DIFFUSE.
// See glColorMaterial.
GLDoubleState st_color_material_parameter(GL_COLOR_MATERIAL_PARAMETER, "GL_COLOR_MATERIAL_PARAMETER", 1);

// params returns one value, the x pixel zoom factor. The initial value is 1. See
// glPixelZoom.
GLDoubleState st_zoom_x(GL_ZOOM_X, "GL_ZOOM_X", 1);

// params returns a single boolean value indicating whether alpha testing of
// fragments is enabled. The initial value is GL_FALSE. See glAlphaFunc.
GLIntegerState st_alpha_test(GL_ALPHA_TEST, "GL_ALPHA_TEST", 1);

// params returns a single boolean value indicating whether the GL is in color
// index mode (GL_TRUE) or RGBA mode (GL_FALSE).
GLIntegerState st_index_mode(GL_INDEX_MODE, "GL_INDEX_MODE", 1);

// params returns one value, the byte offset between consecutive edge flags in the
// edge flag array. The initial value is 0. See glEdgeFlagPointer.
GLIntegerState st_edge_flag_array_stride(GL_EDGE_FLAG_ARRAY_STRIDE, "GL_EDGE_FLAG_ARRAY_STRIDE", 1);

// params returns one value, the depth bias factor used during pixel transfers.
// The initial value is 0. See glPixelTransfer.
GLDoubleState st_depth_bias(GL_DEPTH_BIAS, "GL_DEPTH_BIAS", 1);

// params returns one value, the line width as specified with glLineWidth. The
// initial value is 1.
GLDoubleState st_line_width(GL_LINE_WIDTH, "GL_LINE_WIDTH", 1);

// params returns one value, the y pixel zoom factor. The initial value is 1. See
// glPixelZoom.
GLDoubleState st_zoom_y(GL_ZOOM_Y, "GL_ZOOM_Y", 1);

// params returns one value, the reference value for the alpha test. The initial
// value is 0. See glAlphaFunc. An integer value, if requested, is linearly mapped from
// the internal floating-point representation such that 1.0 returns the most positive
// representable integer value, and -1.0 returns the most negative representable integer value.
GLDoubleState st_alpha_test_ref(GL_ALPHA_TEST_REF, "GL_ALPHA_TEST_REF", 1);

// params returns four values: the red, green, blue, and alpha values used to
// clear the color buffers. Integer values, if requested, are linearly mapped from the
// internal floatingpoint representation such that 1.0 returns the most positive
// representable integer value, and -1.0 returns the most negative representable integer value.
// The initial value is (0, 0, 0, 0). See glClearColor.
GLDoubleState st_color_clear_value(GL_COLOR_CLEAR_VALUE, "GL_COLOR_CLEAR_VALUE", 4);

// params returns one value, the number of rows of pixel locations skipped before
// the first pixel is read from memory. The initial value is 0. See glPixelStore.
GLIntegerState st_unpack_skip_rows(GL_UNPACK_SKIP_ROWS, "GL_UNPACK_SKIP_ROWS", 1);

// params returns a single boolean value indicating if the depth buffer is enabled
// for writing. The initial value is GL_TRUE. See glDepthMask.
GLIntegerState st_depth_writemask(GL_DEPTH_WRITEMASK, "GL_DEPTH_WRITEMASK", 1);

// params returns a single boolean value indicating whether 2D evaluation
// generates normals. The initial value is GL_FALSE. See glMap2.
GLIntegerState st_map2_normal(GL_MAP2_NORMAL, "GL_MAP2_NORMAL", 1);

// params returns one value, a symbolic constant indicating which materials have a
// parameter that is tracking the current color. The initial value is GL_FRONT_AND_BACK. See
// glColorMaterial.
GLIntegerState st_color_material_face(GL_COLOR_MATERIAL_FACE, "GL_COLOR_MATERIAL_FACE", 1);

// params returns one value, the image height used for reading pixel data from
// memory. The initial is 0. See glPixelStore.
GLIntegerState st_unpack_image_height(GL_UNPACK_IMAGE_HEIGHT, "GL_UNPACK_IMAGE_HEIGHT", 1);

// params returns a single boolean value indicating whether 1D evaluation
// generates colors. The initial value is GL_FALSE. See glMap1.
GLIntegerState st_map1_color_4(GL_MAP1_COLOR_4, "GL_MAP1_COLOR_4", 1);

// params returns a single value indicating the number of texture units supported.
// The value must be at least 1. See glActiveTextureARB.
GLIntegerState st_max_texture_units_arb(GL_MAX_TEXTURE_UNITS_ARB, "GL_MAX_TEXTURE_UNITS_ARB", 1);

// params returns one value, the green bias factor applied to RGBA fragments after
// convolution. The initial value is 0. See glPixelTransfer.
GLDoubleState st_post_convolution_green_bias(GL_POST_CONVOLUTION_GREEN_BIAS, "GL_POST_CONVOLUTION_GREEN_BIAS", 1);

// params returns one value, the size of the alpha-to-alpha pixel translation
// table. The initial value is 1. See glPixelMap.
GLIntegerState st_pixel_map_a_to_a_size(GL_PIXEL_MAP_A_TO_A_SIZE, "GL_PIXEL_MAP_A_TO_A_SIZE", 1);

// params returns four values: the red, green, blue, and alpha components of the
// ambient intensity of the entire scene. Integer values, if requested, are linearly
// mapped from the internal floating-point representation such that 1.0 returns the most
// positive representable integer value, and -1.0 returns the most negative representable
// integer value. The initial value is (0.2, 0.2, 0.2, 1.0). See glLightModel.
GLDoubleState st_light_model_ambient(GL_LIGHT_MODEL_AMBIENT, "GL_LIGHT_MODEL_AMBIENT", 4);

// params returns a single boolean value indicating whether histogram is enabled.
// The initial value is GL_FALSE. See glHistogram.
GLIntegerState st_histogram(GL_HISTOGRAM, "GL_HISTOGRAM", 1);

// params returns a single boolean value indicating whether post color matrix
// transformation lookup is enabled. The initial value is GL_FALSE. See glColorTable.
GLIntegerState st_post_color_matrix_color_table(GL_POST_COLOR_MATRIX_COLOR_TABLE, "GL_POST_COLOR_MATRIX_COLOR_TABLE", 1);

// params returns one value, the number of red bitplanes in the accumulation
// buffer.
GLIntegerState st_accum_red_bits(GL_ACCUM_RED_BITS, "GL_ACCUM_RED_BITS", 1);

// params returns four values, the red, green, blue, and alpha values which are
// the components of the blend color. See glBlendColor.
GLDoubleState st_blend_color(GL_BLEND_COLOR, "GL_BLEND_COLOR", 4);

// params returns one value, the number of alpha bitplanes in each color buffer.
GLIntegerState st_alpha_bits(GL_ALPHA_BITS, "GL_ALPHA_BITS", 1);

// params returns a single boolean value indicating whether stippling of lines is
// enabled. The initial value is GL_FALSE. See glLineStipple.
GLIntegerState st_line_stipple(GL_LINE_STIPPLE, "GL_LINE_STIPPLE", 1);

// params returns one value, a symbolic constant indicating the mode of the point
// antialiasing hint. The initial value is GL_DONT_CARE. See glHint.
GLIntegerState st_point_smooth_hint(GL_POINT_SMOOTH_HINT, "GL_POINT_SMOOTH_HINT", 1);

// params returns one value, the offset added to color and stencil indices during
// pixel transfers. The initial value is 0. See glPixelTransfer.
GLIntegerState st_index_offset(GL_INDEX_OFFSET, "GL_INDEX_OFFSET", 1);

// params returns one value indicating the depth of the attribute stack. The
// initial value is 0. See glPushClientAttrib.
GLIntegerState st_client_attrib_stack_depth(GL_CLIENT_ATTRIB_STACK_DEPTH, "GL_CLIENT_ATTRIB_STACK_DEPTH", 1);

// params returns one value, the data type of the coordinates in the texture
// coordinate array. The initial value is GL_FLOAT. See glTexCoordPointer.
GLIntegerState st_texture_coord_array_type(GL_TEXTURE_COORD_ARRAY_TYPE, "GL_TEXTURE_COORD_ARRAY_TYPE", 1);

// params returns one value, a mask indicating which bitplanes of each color index
// buffer can be written. The initial value is all 1's. See glIndexMask.
GLIntegerState st_index_writemask(GL_INDEX_WRITEMASK, "GL_INDEX_WRITEMASK", 1);

// params returns one value. This value is multiplied by an
// implementation-specific value and then added to the depth value of each fragment generated when a
// polygon is rasterized. The initial value is 0. See glPolygonOffset.
GLDoubleState st_polygon_offset_units(GL_POLYGON_OFFSET_UNITS, "GL_POLYGON_OFFSET_UNITS", 1);

// params returns one value, the green scale factor applied to RGBA fragments
// after color matrix transformations. The initial value is 1. See glPixelTransfer.
GLDoubleState st_post_color_matrix_green_scale(GL_POST_COLOR_MATRIX_GREEN_SCALE, "GL_POST_COLOR_MATRIX_GREEN_SCALE", 1);

// params returns one value, the byte offset between consecutive vertices in the
// vertex array. The initial value is 0. See glVertexPointer.
GLIntegerState st_vertex_array_stride(GL_VERTEX_ARRAY_STRIDE, "GL_VERTEX_ARRAY_STRIDE", 1);

// params returns sixteen values: the modelview matrix on the top of the modelview
// matrix stack. Initially this matrix is the identity matrix. See glPushMatrix.
GLDoubleState st_modelview_matrix(GL_MODELVIEW_MATRIX, "GL_MODELVIEW_MATRIX", 16);

// params returns one value, the blue bias factor applied to RGBA fragments after
// color matrix transformations. The initial value is 0. See glPixelTransfer.
GLDoubleState st_post_color_matrix_blue_bias(GL_POST_COLOR_MATRIX_BLUE_BIAS, "GL_POST_COLOR_MATRIX_BLUE_BIAS", 1);

// params returns one value, the number of bitplanes in each color index buffer.
GLIntegerState st_index_bits(GL_INDEX_BITS, "GL_INDEX_BITS", 1);

// params returns one value, a symbolic constant indicating the mode of the fog
// hint. The initial value is GL_DONT_CARE. See glHint.
GLIntegerState st_fog_hint(GL_FOG_HINT, "GL_FOG_HINT", 1);

// params returns one value, the end factor for the linear fog equation. The
// initial value is 1. See glFog.
GLIntegerState st_fog_end(GL_FOG_END, "GL_FOG_END", 1);

// params returns single boolean value indicating whether normal rescaling is
// enabled. See glEnable.
GLIntegerState st_rescale_normal(GL_RESCALE_NORMAL, "GL_RESCALE_NORMAL", 1);

// params returns a single boolean value indicating whether the vertex array is
// enabled. The initial value is GL_FALSE. See glVertexPointer.
GLIntegerState st_vertex_array(GL_VERTEX_ARRAY, "GL_VERTEX_ARRAY", 1);

// params returns one value, the green bias factor applied to RGBA fragments after
// color matrix transformations. The initial value is 0. See glPixelTransfer
GLDoubleState st_post_color_matrix_green_bias(GL_POST_COLOR_MATRIX_GREEN_BIAS, "GL_POST_COLOR_MATRIX_GREEN_BIAS", 1);

// params returns one value, the number of auxiliary color buffers. The initial
// value is 0.
GLIntegerState st_aux_buffers(GL_AUX_BUFFERS, "GL_AUX_BUFFERS", 1);

// params returns one value, the green scale factor applied to RGBA fragments
// after convolution. The initial value is 1. See glPixelTransfer.
GLDoubleState st_post_convolution_green_scale(GL_POST_CONVOLUTION_GREEN_SCALE, "GL_POST_CONVOLUTION_GREEN_SCALE", 1);

// params returns four values: the s, t, r, and q current raster texture
// coordinates. The initial value is (0, 0, 0, 1). See glRasterPos and glTexCoord.
GLDoubleState st_current_raster_texture_coords(GL_CURRENT_RASTER_TEXTURE_COORDS, "GL_CURRENT_RASTER_TEXTURE_COORDS", 4);

// params returns a single boolean value indicating whether polygon stippling is
// enabled. The initial value is GL_FALSE. See glPolygonStipple.
GLIntegerState st_polygon_stipple(GL_POLYGON_STIPPLE, "GL_POLYGON_STIPPLE", 1);

// params returns one value, the byte offset between consecutive normals in the
// normal array. The initial value is 0. See glNormalPointer.
GLIntegerState st_normal_array_stride(GL_NORMAL_ARRAY_STRIDE, "GL_NORMAL_ARRAY_STRIDE", 1);

// params returns a single boolean value indicating whether polygon offset is
// enabled for polygons in point mode. The initial value is GL_FALSE. See glPolygonOffset.
GLIntegerState st_polygon_offset_point(GL_POLYGON_OFFSET_POINT, "GL_POLYGON_OFFSET_POINT", 1);

// params returns one value, the row length used for reading pixel data from
// memory. The initial value is 0. See glPixelStore.
GLIntegerState st_unpack_row_length(GL_UNPACK_ROW_LENGTH, "GL_UNPACK_ROW_LENGTH", 1);

// params returns a single boolean value indicating whether pixel minmax values
// are computed. The initial value is GL_FALSE. See glMinmax.
GLIntegerState st_minmax(GL_MINMAX, "GL_MINMAX", 1);

// params returns two values, the smallest and largest supported widths for
// aliased lines.
GLDoubleState st_aliased_line_width_range(GL_ALIASED_LINE_WIDTH_RANGE, "GL_ALIASED_LINE_WIDTH_RANGE", 2);

// params returns one value. The value gives a rough estimate of the largest
// texture that the GL can handle. If the GL version is 1.1 or greater, use
// GL_PROXY_TEXTURE_1D or GL_PROXY_TEXTURE_2D to determine if a texture is too large. See glTexImage1D
// and glTexImage2D.
GLIntegerState st_max_texture_size(GL_MAX_TEXTURE_SIZE, "GL_MAX_TEXTURE_SIZE", 1);

// params returns one value, the point size as specified by glPointSize. The
// initial value is 1.
GLIntegerState st_point_size(GL_POINT_SIZE, "GL_POINT_SIZE", 1);

// params returns one value, the mask that controls writing of the stencil
// bitplanes. The initial value is all 1's. See glStencilMask.
GLIntegerState st_stencil_writemask(GL_STENCIL_WRITEMASK, "GL_STENCIL_WRITEMASK", 1);

// params returns four values: the s, t, r, and q current texture coordinates. The
// initial value is (0, 0, 0, 1). See glTexCoord.
GLDoubleState st_current_texture_coords(GL_CURRENT_TEXTURE_COORDS, "GL_CURRENT_TEXTURE_COORDS", 4);

// params returns a single boolean value indicating whether polygon offset is
// enabled for polygons in fill mode. The initial value is GL_FALSE. See glPolygonOffset.
GLIntegerState st_polygon_offset_fill(GL_POLYGON_OFFSET_FILL, "GL_POLYGON_OFFSET_FILL", 1);

// params returns one value, the size of the green-to-green pixel translation
// table. The initial value is 1. See glPixelMap.
GLIntegerState st_pixel_map_g_to_g_size(GL_PIXEL_MAP_G_TO_G_SIZE, "GL_PIXEL_MAP_G_TO_G_SIZE", 1);

// params returns one value, the byte offset between consecutive colors in the
// color array. The initial value is 0. See glColorPointer.
GLDoubleState st_color_array_stride(GL_COLOR_ARRAY_STRIDE, "GL_COLOR_ARRAY_STRIDE", 1);

// params returns a single boolean value indicating whether double buffering is
// supported.
GLIntegerState st_doublebuffer(GL_DOUBLEBUFFER, "GL_DOUBLEBUFFER", 1);

// params returns one value, the red bias factor applied to RGBA fragments after
// color matrix transformations. The initial value is 0. See glPixelTransfer.
GLDoubleState st_post_color_matrix_red_bias(GL_POST_COLOR_MATRIX_RED_BIAS, "GL_POST_COLOR_MATRIX_RED_BIAS", 1);

// params returns one value, the maximum supported depth of the selection name
// stack. The value must be at least 64. See glPushName.
GLIntegerState st_max_name_stack_depth(GL_MAX_NAME_STACK_DEPTH, "GL_MAX_NAME_STACK_DEPTH", 1);

// params returns two values: the number of partitions in the 2D map's i and j
// grid domains. The initial value is (1,1). See glMapGrid.
GLIntegerState st_map2_grid_segments(GL_MAP2_GRID_SEGMENTS, "GL_MAP2_GRID_SEGMENTS", 2);

// params returns one value, the fog color index. The initial value is 0. See
// glFog.
GLIntegerState st_fog_index(GL_FOG_INDEX, "GL_FOG_INDEX", 1);

// params returns one value, the number of pixel images skipped before the first
// pixel is read from memory. The initial value is 0. See glPixelStore.
GLIntegerState st_unpack_skip_images(GL_UNPACK_SKIP_IMAGES, "GL_UNPACK_SKIP_IMAGES", 1);

// params returns a single boolean value indicating whether the edge flag array is
// enabled. The initial value is GL_FALSE. See glEdgeFlagPointer.
GLIntegerState st_edge_flag_array(GL_EDGE_FLAG_ARRAY, "GL_EDGE_FLAG_ARRAY", 1);

// params returns one value, the recommended maximum number of vertex array
// indices. See glDrawRangeElements.
GLIntegerState st_max_elements_indices(GL_MAX_ELEMENTS_INDICES, "GL_MAX_ELEMENTS_INDICES", 1);

// params returns one value, the blue scale factor applied to RGBA fragments after
// convolution. The initial value is 1. See glPixelTransfer.
GLDoubleState st_post_convolution_blue_scale(GL_POST_CONVOLUTION_BLUE_SCALE, "GL_POST_CONVOLUTION_BLUE_SCALE", 1);

// params returns one value, the 16-bit line stipple pattern. The initial value is
// all 1's. See glLineStipple.
GLIntegerState st_line_stipple_pattern(GL_LINE_STIPPLE_PATTERN, "GL_LINE_STIPPLE_PATTERN", 1);

// params returns two values: the endpoints of the 1D map's grid domain. The
// initial value is (0, 1). See glMapGrid.
GLIntegerState st_map1_grid_domain(GL_MAP1_GRID_DOMAIN, "GL_MAP1_GRID_DOMAIN", 2);

// params returns one value, the size of the index-to-alpha pixel translation
// table. The initial value is 1. See glPixelMap.
GLIntegerState st_pixel_map_i_to_a_size(GL_PIXEL_MAP_I_TO_A_SIZE, "GL_PIXEL_MAP_I_TO_A_SIZE", 1);

// params returns a single boolean value indicating whether 2D separable
// convolution is enabled. The initial value is GL_FALSE. See glSeparableFilter2D.
GLIntegerState st_separable_2d(GL_SEPARABLE_2D, "GL_SEPARABLE_2D", 1);

// params returns one value, the granularity of sizes for antialiased points. See
// glPointSize.
GLDoubleState st_smooth_point_size_granularity(GL_SMOOTH_POINT_SIZE_GRANULARITY, "GL_SMOOTH_POINT_SIZE_GRANULARITY", 1);

// params returns one value, the granularity of widths for antialiased lines. See
// glLineWidth.
GLDoubleState st_smooth_line_width_granularity(GL_SMOOTH_LINE_WIDTH_GRANULARITY, "GL_SMOOTH_LINE_WIDTH_GRANULARITY", 1);

// params returns a single boolean value indicating whether 3D texture mapping is
// enabled. The initial value is GL_FALSE. See glTexImage3D.
GLIntegerState st_texture_3d(GL_TEXTURE_3D, "GL_TEXTURE_3D", 1);

// params returns one value indicating the maximum supported depth of the client
// attribute stack. See glPushClientAttrib.
GLIntegerState st_max_client_attrib_stack_depth(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, "GL_MAX_CLIENT_ATTRIB_STACK_DEPTH", 1);

// params returns one value, the index to which the stencil bitplanes are cleared.
// The initial value is 0. See glClearStencil.
GLIntegerState st_stencil_clear_value(GL_STENCIL_CLEAR_VALUE, "GL_STENCIL_CLEAR_VALUE", 1);

// params returns a single boolean value indicating whether 2D evaluation
// generates color indices. The initial value is GL_FALSE. See glMap2.
GLIntegerState st_map2_index(GL_MAP2_INDEX, "GL_MAP2_INDEX", 1);

// params returns one value, the number of green bitplanes in each color buffer.
GLIntegerState st_green_bits(GL_GREEN_BITS, "GL_GREEN_BITS", 1);

// params returns a single boolean value indicating whether automatic generation
// of the q texture coordinate is enabled. The initial value is GL_FALSE. See
// glTexGen.
GLIntegerState st_texture_gen_q(GL_TEXTURE_GEN_Q, "GL_TEXTURE_GEN_Q", 1);

// params returns one value, the size of the index-to-green pixel translation
// table. The initial value is 1. See glPixelMap.
GLIntegerState st_pixel_map_i_to_g_size(GL_PIXEL_MAP_I_TO_G_SIZE, "GL_PIXEL_MAP_I_TO_G_SIZE", 1);

// params returns one value, the size of the index-to-red pixel translation table.
// The initial value is 1. See glPixelMap.
GLIntegerState st_pixel_map_i_to_r_size(GL_PIXEL_MAP_I_TO_R_SIZE, "GL_PIXEL_MAP_I_TO_R_SIZE", 1);

// params returns a single boolean value indicating whether the texture coordinate
// array is enabled. The initial value is GL_FALSE. See glTexCoordPointer.
GLIntegerState st_texture_coord_array(GL_TEXTURE_COORD_ARRAY, "GL_TEXTURE_COORD_ARRAY", 1);

// params returns one value, the number of components per color in the color
// array. The initial value is 4. See glColorPointer.
GLIntegerState st_color_array_size(GL_COLOR_ARRAY_SIZE, "GL_COLOR_ARRAY_SIZE", 1);

// params returns one value, the number of pixel locations skipped before the
// first pixel is read from memory. The initial value is 0. See glPixelStore.
GLIntegerState st_unpack_skip_pixels(GL_UNPACK_SKIP_PIXELS, "GL_UNPACK_SKIP_PIXELS", 1);

// params returns one value, the scaling factor used to determine the variable
// offset that is added to the depth value of each fragment generated when a polygon is
// rasterized. The initial value is 0. See glPolygonOffset.
GLDoubleState st_polygon_offset_factor(GL_POLYGON_OFFSET_FACTOR, "GL_POLYGON_OFFSET_FACTOR", 1);

// params returns one value, the red bias factor applied to RGBA fragments after
// convolution. The initial value is 0. See glPixelTransfer.
GLDoubleState st_post_convolution_red_bias(GL_POST_CONVOLUTION_RED_BIAS, "GL_POST_CONVOLUTION_RED_BIAS", 1);

// params returns a single boolean value indicating whether polygon culling is
// enabled. The initial value is GL_FALSE. See glCullFace.
GLIntegerState st_cull_face(GL_CULL_FACE, "GL_CULL_FACE", 1);

// params returns four values: the endpoints of the 2D map's i and j grid domains.
// The initial value is (0,1; 0,1). See glMapGrid.
GLIntegerState st_map2_grid_domain(GL_MAP2_GRID_DOMAIN, "GL_MAP2_GRID_DOMAIN", 4);

// params returns one value, the fog density parameter. The initial value is 1.
// See glFog.
GLDoubleState st_fog_density(GL_FOG_DENSITY, "GL_FOG_DENSITY", 1);

// params returns one value, the number of bitplanes in the stencil buffer.
GLIntegerState st_stencil_bits(GL_STENCIL_BITS, "GL_STENCIL_BITS", 1);

// params returns a single boolean value indicating whether 2D texture mapping is
// enabled. The initial value is GL_FALSE. See glTexImage2D.
GLIntegerState st_texture_2d(GL_TEXTURE_2D, "GL_TEXTURE_2D", 1);

// params returns one value, the maximum number of lights. The value must be at
// least 8. See glLight.
GLIntegerState st_max_lights(GL_MAX_LIGHTS, "GL_MAX_LIGHTS", 1);

// params returns one value, a symbolic constant indicating what action is taken
// when the stencil test passes, but the depth test fails. The initial value is
// GL_KEEP. See glStencilOp.
GLIntegerState st_stencil_pass_depth_fail(GL_STENCIL_PASS_DEPTH_FAIL, "GL_STENCIL_PASS_DEPTH_FAIL", 1);

// params returns one value, the alpha bias factor applied to RGBA fragments after
// convolution. The initial value is 0. See glPixelTransfer.
GLDoubleState st_post_convolution_alpha_bias(GL_POST_CONVOLUTION_ALPHA_BIAS, "GL_POST_CONVOLUTION_ALPHA_BIAS", 1);

// params returns one value, the data type of indexes in the color index array.
// The initial value is GL_FLOAT. See glIndexPointer.
GLIntegerState st_index_array_type(GL_INDEX_ARRAY_TYPE, "GL_INDEX_ARRAY_TYPE", 1);

// params returns a single integer value indicating the current client active
// multitexture unit. The initial value is GL_TEXTURE0_ARB. See glClientActiveTextureARB.
GLIntegerState st_client_active_texture_arb(GL_CLIENT_ACTIVE_TEXTURE_ARB, "GL_CLIENT_ACTIVE_TEXTURE_ARB", 1);

// params returns two values: the maximum supported width and height of the
// viewport. These must be at least as large as the visible dimensions of the display being
// rendered to. See glViewport.
GLIntegerState st_max_viewport_dims(GL_MAX_VIEWPORT_DIMS, "GL_MAX_VIEWPORT_DIMS", 2);

// params returns one value, the name of the display list currently under
// construction. 0 is returned if no display list is currently under construction. The initial
// value is 0. See glNewList.
GLIntegerState st_list_index(GL_LIST_INDEX, "GL_LIST_INDEX", 1);

// params returns one value, the number of rows of pixel locations skipped before
// the first pixel is written into memory. The initial value is 0. See glPixelStore.
GLIntegerState st_pack_skip_rows(GL_PACK_SKIP_ROWS, "GL_PACK_SKIP_ROWS", 1);

// params returns one value, the recommended maximum number of vertex array
// vertices. See glDrawRangeElements.
GLIntegerState st_max_elements_vertices(GL_MAX_ELEMENTS_VERTICES, "GL_MAX_ELEMENTS_VERTICES", 1);

// params returns one value, the number of partitions in the 1D map's grid domain.
// The initial value is 1. See glMapGrid.
GLIntegerState st_map1_grid_segments(GL_MAP1_GRID_SEGMENTS, "GL_MAP1_GRID_SEGMENTS", 1);

// params returns four values: the red, green, blue, and alpha values of the
// current color. Integer values, if requested, are linearly mapped from the internal
// floating-point representation such that 1.0 returns the most positive representable integer
// value, and -1.0 returns the most negative representable integer value. See glColor.
// The initial value is (1, 1, 1, 1).
GLDoubleState st_current_color(GL_CURRENT_COLOR, "GL_CURRENT_COLOR", 4);

// params returns four values: the x and y window coordinates of the viewport,
// followed by its width and height. Initially the x and y window coordinates are both set
// to 0, and the width and height are set to the width and height of the window into
// which the GL will do its rendering. See glViewport.
GLIntegerState st_viewport(GL_VIEWPORT, "GL_VIEWPORT", 4);

// params returns one value, the depth of the attribute stack. If the stack is
// empty, 0 is returned. The initial value is 0. See glPushAttrib.
GLIntegerState st_attrib_stack_depth(GL_ATTRIB_STACK_DEPTH, "GL_ATTRIB_STACK_DEPTH", 1);

// params returns a single boolean value indicating whether the current edge flag
// is GL_TRUE or GL_FALSE. The initial value is GL_TRUE. See glEdgeFlag.
GLIntegerState st_edge_flag(GL_EDGE_FLAG, "GL_EDGE_FLAG", 1);

// params returns a single boolean value indicating whether normals are
// automatically scaled to unit length after they have been transformed to eye coordinates. The
// initial value is GL_FALSE. See glNormal.
GLIntegerState st_normalize(GL_NORMALIZE, "GL_NORMALIZE", 1);

// params returns one value, the size of the index-to-blue pixel translation
// table. The initial value is 1. See glPixelMap.
GLIntegerState st_pixel_map_i_to_b_size(GL_PIXEL_MAP_I_TO_B_SIZE, "GL_PIXEL_MAP_I_TO_B_SIZE", 1);

// params returns one value, the mask that is used to mask both the stencil
// reference value and the stencil buffer value before they are compared. The initial value
// is all 1's. See glStencilFunc.
GLIntegerState st_stencil_value_mask(GL_STENCIL_VALUE_MASK, "GL_STENCIL_VALUE_MASK", 1);

// params returns one value, the symbolic constant identifying the destination
// blend function. The initial value is GL_ZERO. See glBlendFunc.
GLIntegerState st_blend_dst(GL_BLEND_DST, "GL_BLEND_DST", 1);

// params returns one value, the blue bias factor used during pixel transfers. The
// initial value is 0. See glPixelTransfer.
GLDoubleState st_blue_bias(GL_BLUE_BIAS, "GL_BLUE_BIAS", 1);

// params returns a single boolean value indicating whether polygon offset is
// enabled for polygons in line mode. The initial value is GL_FALSE. See glPolygonOffset.
GLIntegerState st_polygon_offset_line(GL_POLYGON_OFFSET_LINE, "GL_POLYGON_OFFSET_LINE", 1);

// params returns a single boolean value indicating whether automatic generation
// of the T texture coordinate is enabled. The initial value is GL_FALSE. See
// glTexGen.
GLIntegerState st_texture_gen_t(GL_TEXTURE_GEN_T, "GL_TEXTURE_GEN_T", 1);

// params returns one value, the symbolic constant identifying the source blend
// function. The initial value is GL_ONE. See glBlendFunc.
GLIntegerState st_blend_src(GL_BLEND_SRC, "GL_BLEND_SRC", 1);

// params returns two values: symbolic constants indicating whether front-facing
// and back-facing polygons are rasterized as points, lines, or filled polygons. The
// initial value is GL_FILL. See glPolygonMode.
GLIntegerState st_polygon_mode(GL_POLYGON_MODE, "GL_POLYGON_MODE", 2);

// params returns a single boolean value indicating whether 2D evaluation
// generates 1D texture coordinates. The initial value is GL_FALSE. See glMap2.
GLIntegerState st_map2_texture_coord_1(GL_MAP2_TEXTURE_COORD_1, "GL_MAP2_TEXTURE_COORD_1", 1);

// params returns a single boolean value indicating whether 2D evaluation
// generates 2D texture coordinates. The initial value is GL_FALSE. See glMap2.
GLIntegerState st_map2_texture_coord_2(GL_MAP2_TEXTURE_COORD_2, "GL_MAP2_TEXTURE_COORD_2", 1);

// params returns a single boolean value indicating whether 2D evaluation
// generates 3D texture coordinates. The initial value is GL_FALSE. See glMap2.
GLIntegerState st_map2_texture_coord_3(GL_MAP2_TEXTURE_COORD_3, "GL_MAP2_TEXTURE_COORD_3", 1);

// params returns a single boolean value indicating whether 2D evaluation
// generates 4D texture coordinates. The initial value is GL_FALSE. See glMap2.
GLIntegerState st_map2_texture_coord_4(GL_MAP2_TEXTURE_COORD_4, "GL_MAP2_TEXTURE_COORD_4", 1);

// params returns one value, a symbolic constant indicating the selected logic
// operation mode. The initial value is GL_COPY. See glLogicOp.
GLIntegerState st_logic_op_mode(GL_LOGIC_OP_MODE, "GL_LOGIC_OP_MODE", 1);

// params returns one value, the maximum supported depth of the modelview matrix
// stack. The value must be at least 32. See glPushMatrix.
GLIntegerState st_max_modelview_stack_depth(GL_MAX_MODELVIEW_STACK_DEPTH, "GL_MAX_MODELVIEW_STACK_DEPTH", 1);

// params returns one value, a symbolic constant indicating which fog equation is
// selected. The initial value is GL_EXP. See glFog.
GLIntegerState st_fog_mode(GL_FOG_MODE, "GL_FOG_MODE", 1);

// params returns one value, a symbolic constant indicating the mode of the line
// antialiasing hint. The initial value is GL_DONT_CARE. See glHint.
GLIntegerState st_line_smooth_hint(GL_LINE_SMOOTH_HINT, "GL_LINE_SMOOTH_HINT", 1);

// params returns one value, the byte offset between consecutive color indexes in
// the color index array. The initial value is 0. See glIndexPointer.
GLIntegerState st_index_array_stride(GL_INDEX_ARRAY_STRIDE, "GL_INDEX_ARRAY_STRIDE", 1);

// params returns one value, the number of red bitplanes in each color buffer.
GLIntegerState st_red_bits(GL_RED_BITS, "GL_RED_BITS", 1);

// params returns one value, the image height used for writing pixel data to
// memory. The initial value is 0. See glPixelStore.
GLIntegerState st_pack_image_height(GL_PACK_IMAGE_HEIGHT, "GL_PACK_IMAGE_HEIGHT", 1);

// params returns a single boolean value indicating whether fogging is enabled.
// The initial value is GL_FALSE. See glFog.
GLIntegerState st_fog(GL_FOG, "GL_FOG", 1);

// params returns a single boolean value indicating whether 2D evaluation
// generates 3D vertex coordinates. The initial value is GL_FALSE. See glMap2.
GLIntegerState st_map2_vertex_3(GL_MAP2_VERTEX_3, "GL_MAP2_VERTEX_3", 1);

// params returns a single boolean value indicating whether 2D evaluation
// generates 4D vertex coordinates. The initial value is GL_FALSE. See glMap2.
GLIntegerState st_map2_vertex_4(GL_MAP2_VERTEX_4, "GL_MAP2_VERTEX_4", 1);

// params returns one value, the size of the blue-to-blue pixel translation table.
// The initial value is 1. See glPixelMap.
GLIntegerState st_pixel_map_b_to_b_size(GL_PIXEL_MAP_B_TO_B_SIZE, "GL_PIXEL_MAP_B_TO_B_SIZE", 1);

// params returns one value, the byte alignment used for writing pixel data to
// memory. The initial value is 4. See glPixelStore.
GLIntegerState st_pack_alignment(GL_PACK_ALIGNMENT, "GL_PACK_ALIGNMENT", 1);

// params returns one value, an estimate of the number of bits of subpixel
// resolution that are used to position rasterized geometry in window coordinates. The
// initial value is 4.
GLIntegerState st_subpixel_bits(GL_SUBPIXEL_BITS, "GL_SUBPIXEL_BITS", 1);

// params returns one value, a symbolic constant indicating which polygon faces
// are to be culled. The initial value is GL_BACK. See glCullFace.
GLIntegerState st_cull_face_mode(GL_CULL_FACE_MODE, "GL_CULL_FACE_MODE", 1);

// params returns one value, the number of green bitplanes in the accumulation
// buffer.
GLIntegerState st_accum_green_bits(GL_ACCUM_GREEN_BITS, "GL_ACCUM_GREEN_BITS", 1);

// params returns a single boolean value, indicating whether the normal array is
// enabled. The initial value is GL_FALSE. See glNormalPointer.
GLIntegerState st_normal_array(GL_NORMAL_ARRAY, "GL_NORMAL_ARRAY", 1);

// params returns a single boolean value indicating whether single-bit pixels
// being read from memory are read first from the least significant bit of each unsigned
// byte. The initial value is GL_FALSE. See glPixelStore.
GLIntegerState st_unpack_lsb_first(GL_UNPACK_LSB_FIRST, "GL_UNPACK_LSB_FIRST", 1);

// params returns a single boolean value indicating whether 2D evaluation
// generates colors. The initial value is GL_FALSE. See glMap2.
GLIntegerState st_map2_color_4(GL_MAP2_COLOR_4, "GL_MAP2_COLOR_4", 1);

// params returns a single boolean value indicating whether the color index array
// is enabled. The initial value is GL_FALSE. See glIndexPointer.
GLIntegerState st_index_array(GL_INDEX_ARRAY, "GL_INDEX_ARRAY", 1);

// params returns a single boolean value indicating whether 1D evaluation
// generates normals. The initial value is GL_FALSE. See glMap1.
GLIntegerState st_map1_normal(GL_MAP1_NORMAL, "GL_MAP1_NORMAL", 1);

// params returns sixteen values: the color matrix on the top of the color matrix
// stack. Initially this matrix is the identity matrix. See glPushMatrix.
GLDoubleState st_color_matrix(GL_COLOR_MATRIX, "GL_COLOR_MATRIX", 16);

// params returns a single value, the name of the texture currently bound to the
// target GL_TEXTURE_2D. The initial value is 0. See glBindTexture.
GLIntegerState st_texture_binding_2d(GL_TEXTURE_BINDING_2D, "GL_TEXTURE_BINDING_2D", 1);

// params returns one value, the maximum equation order supported by 1D and 2D
// evaluators. The value must be at least 8. See glMap1 and glMap2.
GLIntegerState st_max_eval_order(GL_MAX_EVAL_ORDER, "GL_MAX_EVAL_ORDER", 1);

// params returns one value, the size of the redto-red pixel translation table.
// The initial value is 1. See glPixelMap.
GLIntegerState st_pixel_map_r_to_r_size(GL_PIXEL_MAP_R_TO_R_SIZE, "GL_PIXEL_MAP_R_TO_R_SIZE", 1);

// params returns one value, the base offset added to all names in arrays
// presented to glCallLists. The initial value is 0. See glListBase.
GLIntegerState st_list_base(GL_LIST_BASE, "GL_LIST_BASE", 1);

// params returns a single boolean value indicating whether single-bit pixels
// being written to memory are written first to the least significant bit of each
// unsigned byte. The initial value is GL_FALSE. See glPixelStore.
GLIntegerState st_pack_lsb_first(GL_PACK_LSB_FIRST, "GL_PACK_LSB_FIRST", 1);

// params returns one value, the alpha bias factor applied to RGBA fragments after
// color matrix transformations. The initial value is 0. See glPixelTransfer.
GLDoubleState st_post_color_matrix_alpha_bias(GL_POST_COLOR_MATRIX_ALPHA_BIAS, "GL_POST_COLOR_MATRIX_ALPHA_BIAS", 1);

// params returns two values, the smallest and largest supported widths for
// antialiased lines. See glLineWidth.
GLDoubleState st_smooth_line_width_range(GL_SMOOTH_LINE_WIDTH_RANGE, "GL_SMOOTH_LINE_WIDTH_RANGE", 2);

// params returns four boolean values: the red, green, blue, and alpha write
// enables for the color buffers. The initial value is (GL_TRUE, GL_TRUE, GL_TRUE,
// GL_TRUE). See glColorMask.
GLIntegerState st_color_writemask(GL_COLOR_WRITEMASK, "GL_COLOR_WRITEMASK", 4);

// params returns one value, the green scale factor used during pixel transfers.
// The initial value is 1. See glPixelTransfer.
GLDoubleState st_green_scale(GL_GREEN_SCALE, "GL_GREEN_SCALE", 1);

// params returns a single boolean value indicating whether a fragment's RGBA
// color values are merged into the framebuffer using a logical operation. The initial
// value is GL_FALSE. See glLogicOp.
GLIntegerState st_color_logic_op(GL_COLOR_LOGIC_OP, "GL_COLOR_LOGIC_OP", 1);

// params returns one value, the symbolic constant that indicates the depth
// comparison function. The initial value is GL_LESS. See glDepthFunc.
GLIntegerState st_depth_func(GL_DEPTH_FUNC, "GL_DEPTH_FUNC", 1);

// params returns one value, the number of pixel images skipped before the first
// pixel is written into memory. The initial value is 0. See glPixelStore.
GLIntegerState st_pack_skip_images(GL_PACK_SKIP_IMAGES, "GL_PACK_SKIP_IMAGES", 1);

// params returns one value, a symbolic constant indicating which buffers are
// being drawn to. See glDrawBuffer. The initial value is GL_BACK if there are back
// buffers, otherwise it is GL_FRONT.
GLIntegerState st_draw_buffer(GL_DRAW_BUFFER, "GL_DRAW_BUFFER", 1);

// params returns a single boolean value indicating whether a fragment's index
// values are merged into the framebuffer using a logical operation. The initial value is
// GL_FALSE. See glLogicOp.
GLIntegerState st_index_logic_op(GL_INDEX_LOGIC_OP, "GL_INDEX_LOGIC_OP", 1);

// params returns one value, the blue scale factor applied to RGBA fragments after
// color matrix transformations. The initial value is 1. See glPixelTransfer.
GLDoubleState st_post_color_matrix_blue_scale(GL_POST_COLOR_MATRIX_BLUE_SCALE, "GL_POST_COLOR_MATRIX_BLUE_SCALE", 1);

// params returns one value, a symbolic constant indicating whether the shading
// mode is flat or smooth. The initial value is GL_SMOOTH. See glShadeModel.
GLIntegerState st_shade_model(GL_SHADE_MODEL, "GL_SHADE_MODEL", 1);

// params returns two values, the smallest and largest supported sizes for aliased
// points.
GLDoubleState st_aliased_point_size_range(GL_ALIASED_POINT_SIZE_RANGE, "GL_ALIASED_POINT_SIZE_RANGE", 2);

// params returns a single value indicating the active multitexture unit. The
// initial value is GL_TEXTURE0_ARB. See glActiveTextureARB.
GLIntegerState st_active_texture_arb(GL_ACTIVE_TEXTURE_ARB, "GL_ACTIVE_TEXTURE_ARB", 1);

// params returns one value, a symbolic constant indicating what action is taken
// when the stencil test fails. The initial value is GL_KEEP. See glStencilOp.
GLIntegerState st_stencil_fail(GL_STENCIL_FAIL, "GL_STENCIL_FAIL", 1);

// params returns one value, the maximum supported depth of the color matrix
// stack. The value must be at least 2. See glPushMatrix.
GLIntegerState st_max_color_matrix_stack_depth(GL_MAX_COLOR_MATRIX_STACK_DEPTH, "GL_MAX_COLOR_MATRIX_STACK_DEPTH", 1);

// params returns one value, the number of bitplanes in the depth buffer.
GLIntegerState st_depth_bits(GL_DEPTH_BITS, "GL_DEPTH_BITS", 1);

// params returns a single value, the name of the texture currently bound to the
// target GL_TEXTURE_1D. The initial value is 0. See glBindTexture.
GLIntegerState st_texture_binding_1d(GL_TEXTURE_BINDING_1D, "GL_TEXTURE_BINDING_1D", 1);

// params returns a single boolean value indicating whether 1D texture mapping is
// enabled. The initial value is GL_FALSE. See glTexImage1D.
GLIntegerState st_texture_1d(GL_TEXTURE_1D, "GL_TEXTURE_1D", 1);

// params returns a single boolean value indicating if colors and color indices
// are to be replaced by table lookup during pixel transfers. The initial value is
// GL_FALSE. See glPixelTransfer.
GLIntegerState st_map_color(GL_MAP_COLOR, "GL_MAP_COLOR", 1);

// params returns one value, a symbolic constant indicating the mode of the
// perspective correction hint. The initial value is GL_DONT_CARE. See glHint.
GLIntegerState st_perspective_correction_hint(GL_PERSPECTIVE_CORRECTION_HINT, "GL_PERSPECTIVE_CORRECTION_HINT", 1);

// params returns one value, the green bias factor used during pixel transfers.
// The initial value is 0.
GLDoubleState st_green_bias(GL_GREEN_BIAS, "GL_GREEN_BIAS", 1);

// params returns a single boolean value indicating whether depth testing of
// fragments is enabled. The initial value is GL_FALSE. See glDepthFunc and glDepthRange.
GLIntegerState st_depth_test(GL_DEPTH_TEST, "GL_DEPTH_TEST", 1);

// params returns one value, the alpha scale factor applied to RGBA fragments
// after convolution. The initial value is 1. See glPixelTransfer.
GLDoubleState st_post_convolution_alpha_scale(GL_POST_CONVOLUTION_ALPHA_SCALE, "GL_POST_CONVOLUTION_ALPHA_SCALE", 1);

// params returns one value, the start factor for the linear fog equation. The
// initial value is 0. See glFog.
GLIntegerState st_fog_start(GL_FOG_START, "GL_FOG_START", 1);

// params returns a single boolean value indicating whether the color table lookup
// is enabled. See glColorTable.
GLIntegerState st_color_table(GL_COLOR_TABLE, "GL_COLOR_TABLE", 1);

// params returns two values: the smallest and largest supported widths for
// antialiased lines. See glLineWidth.
GLDoubleState st_line_width_range(GL_LINE_WIDTH_RANGE, "GL_LINE_WIDTH_RANGE", 2);

// params returns a single boolean value indicating whether 2D map evaluation
// automatically generates surface normals. The initial value is GL_FALSE. See glMap2.
GLIntegerState st_auto_normal(GL_AUTO_NORMAL, "GL_AUTO_NORMAL", 1);

// params returns one value, the maximum number of application-defined clipping
// planes. The value must be at least 6. See glClipPlane.
GLIntegerState st_max_clip_planes(GL_MAX_CLIP_PLANES, "GL_MAX_CLIP_PLANES", 1);

// params returns one value, the red bias factor used during pixel transfers. The
// initial value is 0.
GLDoubleState st_red_bias(GL_RED_BIAS, "GL_RED_BIAS", 1);

// params returns one value, the number of matrices on the projection matrix
// stack. The initial value is 1. See glPushMatrix.
GLIntegerState st_projection_stack_depth(GL_PROJECTION_STACK_DEPTH, "GL_PROJECTION_STACK_DEPTH", 1);

// params returns one value, the data type of each component in the color array.
// The initial value is GL_FLOAT. See glColorPointer.
GLDoubleState st_color_array_type(GL_COLOR_ARRAY_TYPE, "GL_COLOR_ARRAY_TYPE", 1);

// params returns one value, the number of blue bitplanes in each color buffer.
GLIntegerState st_blue_bits(GL_BLUE_BITS, "GL_BLUE_BITS", 1);

// params returns a single boolean value indicating whether 1D evaluation
// generates 4D vertex coordinates. The initial value is GL_FALSE. See glMap1.
GLIntegerState st_map1_vertex_4(GL_MAP1_VERTEX_4, "GL_MAP1_VERTEX_4", 1);

// params returns sixteen values: the texture matrix on the top of the texture
// matrix stack. Initially this matrix is the identity matrix. See glPushMatrix.
GLIntegerState st_texture_matrix(GL_TEXTURE_MATRIX, "GL_TEXTURE_MATRIX", 16);

// params returns one value, the red scale factor used during pixel transfers. The
// initial value is 1. See glPixelTransfer.
GLDoubleState st_red_scale(GL_RED_SCALE, "GL_RED_SCALE", 1);

// params returns one value, a symbolic constant indicating whether the GL is in
// render, select, or feedback mode. The initial value is GL_RENDER. See glRenderMode.
GLIntegerState st_render_mode(GL_RENDER_MODE, "GL_RENDER_MODE", 1);

// params returns a single boolean value indicating whether 1D evaluation
// generates 3D vertex coordinates. The initial value is GL_FALSE. See glMap1.
GLIntegerState st_map1_vertex_3(GL_MAP1_VERTEX_3, "GL_MAP1_VERTEX_3", 1);

// params returns a single boolean value indicating whether the bytes of two-byte
// and fourbyte pixel indices and components are swapped before being written to
// memory. The initial value is GL_FALSE. See glPixelStore.
GLIntegerState st_pack_swap_bytes(GL_PACK_SWAP_BYTES, "GL_PACK_SWAP_BYTES", 1);

// params returns a single value, the name of the texture currently bound to the
// target GL_TEXTURE_3D. The initial value is 0. See glBindTexture.
GLIntegerState st_texture_binding_3d(GL_TEXTURE_BINDING_3D, "GL_TEXTURE_BINDING_3D", 1);

// params returns one value, the blue scale factor used during pixel transfers.
// The initial value is 1. See glPixelTransfer.
GLDoubleState st_blue_scale(GL_BLUE_SCALE, "GL_BLUE_SCALE", 1);

// params returns a single boolean value indicating whether the current raster
// position is valid. The initial value is GL_TRUE. See glRasterPos.
GLIntegerState st_current_raster_position_valid(GL_CURRENT_RASTER_POSITION_VALID, "GL_CURRENT_RASTER_POSITION_VALID", 1);

// params returns one value, a symbolic constant indicating which matrix stack is
// currently the target of all matrix operations. The initial value is GL_MODELVIEW. See
// glMatrixMode.
GLIntegerState st_matrix_mode(GL_MATRIX_MODE, "GL_MATRIX_MODE", 1);

// params returns one value, the size of the index-to-index pixel translation
// table. The initial value is 1. See glPixelMap.
GLIntegerState st_pixel_map_i_to_i_size(GL_PIXEL_MAP_I_TO_I_SIZE, "GL_PIXEL_MAP_I_TO_I_SIZE", 1);

// params returns four values: the x and y window coordinates of the scissor box,
// followed by its width and height. Initially the x and y window coordinates are both 0
// and the width and height are set to the size of the window. See glScissor.
GLIntegerState st_scissor_box(GL_SCISSOR_BOX, "GL_SCISSOR_BOX", 4);

// params returns one value, the color index of the current raster position. The
// initial value is 1. See glRasterPos.
GLIntegerState st_current_raster_index(GL_CURRENT_RASTER_INDEX, "GL_CURRENT_RASTER_INDEX", 1);

// params returns one value, the maximum supported depth of the projection matrix
// stack. The value must be at least 2. See glPushMatrix.
GLIntegerState st_max_projection_stack_depth(GL_MAX_PROJECTION_STACK_DEPTH, "GL_MAX_PROJECTION_STACK_DEPTH", 1);

// params returns one value, the size of the feedback buffer. See
// glFeedbackBuffer.
GLIntegerState st_feedback_buffer_size(GL_FEEDBACK_BUFFER_SIZE, "GL_FEEDBACK_BUFFER_SIZE", 1);

// params returns two values: the near and far mapping limits for the depth
// buffer. Integer values, if requested, are linearly mapped from the internal
// floating-point representation such that 1.0 returns the most positive representable integer
// value, and -1.0 returns the most negative representable integer value. The initial
// value is (0, 1). See glDepthRange.
GLDoubleState st_depth_range(GL_DEPTH_RANGE, "GL_DEPTH_RANGE", 2);

// params returns a single boolean value indicating if stencil indices are to be
// replaced by table lookup during pixel transfers. The initial value is GL_FALSE. See
// glPixelTransfer.
GLIntegerState st_map_stencil(GL_MAP_STENCIL, "GL_MAP_STENCIL", 1);

// params returns a single boolean value indicating whether the bytes of two-byte
// and fourbyte pixel indices and components are swapped after being read from memory.
// The initial value is GL_FALSE. See glPixelStore.
GLIntegerState st_unpack_swap_bytes(GL_UNPACK_SWAP_BYTES, "GL_UNPACK_SWAP_BYTES", 1);

// params returns one value, the maximum supported depth of the projection matrix
// stack. The value must be at least 2. See glPushMatrix.
GLDoubleState st_color_matrix_stack_depth(GL_COLOR_MATRIX_STACK_DEPTH, "GL_COLOR_MATRIX_STACK_DEPTH", 1);

// params returns sixteen values: the projection matrix on the top of the
// projection matrix stack. Initially this matrix is the identity matrix. See glPushMatrix.
GLDoubleState st_projection_matrix(GL_PROJECTION_MATRIX, "GL_PROJECTION_MATRIX", 16);

// params returns one value, a symbolic constant indicating the mode of the
// polygon antialiasing hint. The initial value is GL_DONT_CARE. See glHint.
GLIntegerState st_polygon_smooth_hint(GL_POLYGON_SMOOTH_HINT, "GL_POLYGON_SMOOTH_HINT", 1);

// params returns one value, a symbolic constant indicating whether clockwise or
// counterclockwise polygon winding is treated as front-facing. The initial value is GL_CCW. See
// glFrontFace.
GLIntegerState st_front_face(GL_FRONT_FACE, "GL_FRONT_FACE", 1);

// params returns one value, the maximum supported depth of the attribute stack.
// The value must be at least 16. See glPushAttrib.
GLIntegerState st_max_attrib_stack_depth(GL_MAX_ATTRIB_STACK_DEPTH, "GL_MAX_ATTRIB_STACK_DEPTH", 1);

// params returns one value, the size of the selection buffer. See glSelectBuffer.
GLIntegerState st_selection_buffer_size(GL_SELECTION_BUFFER_SIZE, "GL_SELECTION_BUFFER_SIZE", 1);

// params returns four values: the red, green, blue, and alpha values of the
// current raster position. Integer values, if requested, are linearly mapped from the
// internal floatingpoint representation such that 1.0 returns the most positive
// representable integer value, and -1.0 returns the most negative representable integer value.
// The initial value is (1, 1, 1, 1). See glRasterPos.
GLDoubleState st_current_raster_color(GL_CURRENT_RASTER_COLOR, "GL_CURRENT_RASTER_COLOR", 4);

// params returns four values: the red, green, blue, and alpha components of the
// fog color. Integer values, if requested, are linearly mapped from the internal
// floating-point representation such that 1.0 returns the most positive representable integer
// value, and -1.0 returns the most negative representable integer value. The initial
// value is (0, 0, 0, 0). See glFog.
GLDoubleState st_fog_color(GL_FOG_COLOR, "GL_FOG_COLOR", 4);

// params returns one value, the byte alignment used for reading pixel data from
// memory. The initial value is 4. See glPixelStore.
GLIntegerState st_unpack_alignment(GL_UNPACK_ALIGNMENT, "GL_UNPACK_ALIGNMENT", 1);

// params returns one value, the size of the stencil-to-stencil pixel translation
// table. The initial value is 1. See glPixelMap.
GLIntegerState st_pixel_map_s_to_s_size(GL_PIXEL_MAP_S_TO_S_SIZE, "GL_PIXEL_MAP_S_TO_S_SIZE", 1);


void dump_gl_state(ostream& o) {
    cout << "--------- GL STATE DUMP -----------"<<endl;
    GLState::dumpall(o);
    cout << "--------- END STATE DUMP -----------"<<endl;
}



