#include "core.h"
#include "program.h"
#include "render_api.h"

typedef struct vert_t vert_t;
struct vert_t {
    Vec2 pos;
    Vec2 uv;
};

static Quad quad_init(void) {
    vert_t verts[] = {
        { vec2(-0.5f, -0.5f), vec2(0.0f, 0.0f) },
        { vec2( 0.5f, -0.5f), vec2(1.0f, 0.0f) },
        { vec2(-0.5f,  0.5f), vec2(0.0f, 1.0f) },
        { vec2( 0.5f,  0.5f), vec2(1.0f, 1.0f) },
    };
    vertex_buffer_t vb = vertex_buffer_create(verts, sizeof(verts), BUFFER_USAGE_STATIC);

    u32 indices[] = {
        0, 1, 2,
        2, 3, 1,
    };
    index_buffer_t ib = index_buffer_create(indices, arr_len(indices), BUFFER_USAGE_STATIC);

    pipeline_t pipeline = pipeline_create((pipeline_desc_t) {
            .blend = {
                .enabled = true,
                .color_op = BLEND_OP_ADD,
                .src_color_factor = BLEND_FACTOR_SRC_ALPHA,
                .dst_color_factor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .alpha_op = BLEND_OP_ADD,
                .src_alpha_factor = BLEND_FACTOR_ONE,
                .dst_alpha_factor = BLEND_FACTOR_ZERO,
            },
            .vertex_buffer = vb,
            .vertex_layout = {
                .stride = sizeof(vert_t),
                .attribs = (vertex_attribute_t[]) {
                    [0] = {
                        .type = VERTEX_ATTRIB_TYPE_F32,
                        .count = 2,
                        .offset = offset(vert_t, pos),
                    },
                    [1] = {
                        .type = VERTEX_ATTRIB_TYPE_F32,
                        .count = 2,
                        .offset = offset(vert_t, uv),
                    },
                },
                .attrib_count = 2,
            },
        });

    return (Quad) {
        .vb = vb,
        .ib = ib,
        .pipe = pipeline,
    };
}

static void draw_quad(Quad quad) {
    pipeline_bind(quad.pipe);
    index_buffer_bind(quad.ib);
    draw_indexed(quad.ib.count, 0);
}

app_t* app_init(void) {
    arena_t* arena = arena_new(1<<30);
    app_t* app = arena_push_type(arena, app_t);

    str_t vert = str_read_file(arena, str_lit("assets/shaders/vert.glsl"));
    str_t frag = str_read_file(arena, str_lit("assets/shaders/frag.glsl"));

    texture_t white_texture = texture_create((texture_desc_t) {
            .data = (u8[]) {255, 255, 255, 255},
            .width = 1,
            .height = 1,
            .format = TEXTURE_FORMAT_RGBA_U8,
            .sampler = TEXTURE_SAMPLER_LINEAR,
        });

    texture_t obj_render_target = texture_create((texture_desc_t) {
            .data = NULL,
            .width = 800,
            .height = 600,
            .format = TEXTURE_FORMAT_RGBA_U8,
            .sampler = TEXTURE_SAMPLER_LINEAR,
        });

    *app = (app_t) {
        .arena = arena,

        .quad = quad_init(),
        .obj_shader = shader_create(vert, frag),
        .white_texture = white_texture,

        .obj_render_target = obj_render_target,
        .obj_pass = render_pass_create((render_pass_desc_t) {
                // Target the swapchain
                .target = obj_render_target,
                .load_op = LOAD_OP_CLEAR,
                .clear_color = COLOR_BLACK,
            }),
        .screen_pass = render_pass_create((render_pass_desc_t) {
                // Target the swapchain
                .target = {0},
                .load_op = LOAD_OP_CLEAR,
                .clear_color = COLOR_BLACK,
            }),
    };

    return app;
}

void app_shutdown(app_t* app) {
    arena_free(app->arena);
}

void app_update(app_t* app) {
    const f32 aspect = 800.0f / 600.0f;
    const f32 zoom = 5.0f;
    Mat4 proj = mat4_ortho_projection(-aspect*zoom, aspect*zoom, zoom, -zoom, 1.0f, -1.0f);

    RENDER_PASS(&app->obj_pass) {
        Vec4 ambient = vec4(vec3_arg(vec3s(0.1)), 1.0);

        {
            Mat4 transform = MAT4_IDENTITY;
            transform = mat4_translate(transform, vec3(1.0f, 0.0f, 0.0f));
            transform = mat4_scale(transform, vec3(1.0f, 1.0f, 1.0f));

            texture_bind(app->white_texture, 0);
            shader_use(app->obj_shader);
            color_t color = color_rgb_hex(0xff00ff);
            Vec4 v4_color = *(Vec4 *) &color;
            shader_uniform_vec4(app->obj_shader, "color", v4_color);
            shader_uniform_mat4(app->obj_shader, "proj", proj);
            shader_uniform_mat4(app->obj_shader, "transform", transform);

            shader_uniform_vec4(app->obj_shader, "ambient", ambient);

            draw_quad(app->quad);
        }

        {
            Mat4 transform = MAT4_IDENTITY;
            transform = mat4_translate(transform, vec3(-1.0f, 0.0f, 0.0f));
            transform = mat4_scale(transform, vec3(1.0f, 1.0f, 1.0f));

            texture_bind(app->white_texture, 0);
            shader_use(app->obj_shader);
            color_t color = color_rgb_hex(0x00ff00);
            Vec4 v4_color = *(Vec4 *) &color;
            shader_uniform_vec4(app->obj_shader, "color", v4_color);
            shader_uniform_mat4(app->obj_shader, "proj", proj);
            shader_uniform_mat4(app->obj_shader, "transform", transform);

            shader_uniform_vec4(app->obj_shader, "ambient", ambient);

            draw_quad(app->quad);
        }
    }

    RENDER_PASS(&app->screen_pass) {
            Mat4 transform = MAT4_IDENTITY;
            transform = mat4_scale(transform, vec3(2.0f, 2.0f, 1.0f));

            texture_bind(app->obj_render_target, 0);
            shader_use(app->obj_shader);
            shader_uniform_vec4(app->obj_shader, "color", vec4s(1.0f));
            shader_uniform_mat4(app->obj_shader, "proj", MAT4_IDENTITY);
            shader_uniform_mat4(app->obj_shader, "transform", transform);

            shader_uniform_vec4(app->obj_shader, "ambient", vec4s(1.0f));

            draw_quad(app->quad);
    }
}
