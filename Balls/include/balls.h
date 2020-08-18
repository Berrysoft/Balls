#ifndef BALLS_H
#define BALLS_H

#include <xaml/buffer.h>
#include <xaml/enumerable.h>
#include <xaml/meta/meta_macros.h>
#include <xaml/ui/drawing.h>

typedef struct balls_ball balls_ball;

struct balls_ball
{
    xaml_point pos;
    xaml_point speed;
};

XAML_TYPE(balls_ball, { 0x7185ab37, 0xe6a0, 0x418b, { 0x9a, 0xc3, 0x6e, 0x4f, 0x74, 0xcc, 0xea, 0x74 } })

XAML_CONSTEXPR_VAR XAML_STD int32_t balls_radius = 20;
XAML_CONSTEXPR_VAR XAML_STD int32_t balls_max_columns = 6;
XAML_CONSTEXPR_VAR XAML_STD int32_t balls_max_rows = 8;
XAML_CONSTEXPR_VAR XAML_STD int32_t balls_side_length = 200;
XAML_CONSTEXPR_VAR XAML_STD int32_t balls_client_width = balls_side_length * balls_max_columns - 1;
XAML_CONSTEXPR_VAR XAML_STD int32_t balls_client_height = balls_side_length * balls_max_rows - 1;
XAML_CONSTEXPR_VAR double balls_abs_speed = 50.0;
XAML_CONSTEXPR_VAR XAML_STD int32_t balls_num_size = 100;

typedef enum balls_difficulty
{
    balls_difficulty_simple,
    balls_difficulty_normal,
    balls_difficulty_hard
} balls_difficulty;

XAML_TYPE(balls_difficulty, { 0x7471241e, 0x2381, 0x4db2, { 0x85, 0x42, 0xd5, 0xe9, 0x26, 0x47, 0x22, 0xe4 } })

typedef enum balls_special_num
{
    balls_special_new_ball = -1,
    balls_special_delete_ball = -2,
    balls_special_random_turn = -3,
    balls_special_random_turn_old = -4,
    balls_special_double_score = -5
} balls_special_num;

XAML_TYPE(balls_special_num, { 0x7b2a1bd7, 0x2114, 0x4616, { 0x9d, 0x97, 0xf2, 0x2a, 0xf2, 0x02, 0x7c, 0x88 } })

typedef struct balls_map_enumerator_internal balls_map_enumerator_internal;

struct balls_map_enumerator_internal
{
    XAML_STD int32_t ball_num;
    XAML_STD int32_t stopped_num;
    XAML_STD int32_t loop;
};

XAML_TYPE(balls_map_enumerator_internal, { 0x4a0f5f82, 0x898f, 0x46b7, { 0xbc, 0xe6, 0xbe, 0x10, 0x49, 0x6d, 0x25, 0xb0 } })

XAML_CLASS(balls_map_enumerator, { 0x4d8f6447, 0x6046, 0x4831, { 0xb4, 0x97, 0xc4, 0xe7, 0x47, 0xad, 0x3d, 0x1e } })

#define BALLS_MAP_ENUMERATOR(type)                 \
    XAML_VTBL_INHERIT(XAML_ENUMERATOR_VTBL(type)); \
    XAML_METHOD(get_is_end_shooting, type, bool*); \
    XAML_PROP(internal, type, balls_map_enumerator_internal*, balls_map_enumerator_internal XAML_CONST_REF)

XAML_DECL_INTERFACE_(balls_map_enumerator, xaml_enumerator)
{
    XAML_DECL_VTBL(balls_map_enumerator, BALLS_MAP_ENUMERATOR);
};

typedef XAML_STD int32_t balls_map_t[balls_max_rows][balls_max_columns];

XAML_CLASS(balls_map, { 0x8f267939, 0x7dd5, 0x47d8, { 0xb5, 0xe1, 0x20, 0x32, 0xf5, 0x22, 0xa7, 0x1e } })

#define BALLS_MAP_VTBL(type)                                                        \
    XAML_VTBL_INHERIT(XAML_OBJECT_VTBL(type));                                      \
    XAML_PROP(ball_num, type, XAML_STD int32_t*, XAML_STD int32_t);                 \
    XAML_PROP(remain_ball_num, type, XAML_STD int32_t*, XAML_STD int32_t);          \
    XAML_PROP(score, type, XAML_STD uint64_t*, XAML_STD uint64_t);                  \
    XAML_PROP(difficulty, type, balls_difficulty*, balls_difficulty);               \
    XAML_EVENT(ball_score_changed, type);                                           \
    XAML_PROP(start_position, type, xaml_point*, xaml_point XAML_CONST_REF);        \
    XAML_PROP(end_position, type, xaml_point*, xaml_point XAML_CONST_REF);          \
    XAML_PROP(start_velocity, type, xaml_point*, xaml_point XAML_CONST_REF);        \
    XAML_PROP(sample_position, type, xaml_point*, xaml_point XAML_CONST_REF);       \
    XAML_METHOD(get_is_double_score, type, bool*);                                  \
    XAML_METHOD(get_map, type, xaml_vector**);                                      \
    XAML_METHOD(get_is_over, type, bool*);                                          \
    XAML_METHOD(set_sample, type, xaml_point XAML_CONST_REF);                       \
    XAML_METHOD(get_map, type, balls_map_t const**);                                \
    XAML_METHOD(start, type, balls_map_enumerator**);                               \
    XAML_METHOD(start_by, type, xaml_point XAML_CONST_REF, balls_map_enumerator**); \
    XAML_METHOD(reset, type, bool*);                                                \
    XAML_METHOD(reset_all, type);                                                   \
    XAML_METHOD(serialize, type, balls_map_enumerator*, xaml_buffer**);             \
    XAML_METHOD(deserialize, type, xaml_buffer*, balls_map_enumerator**)

XAML_DECL_INTERFACE_(balls_map, xaml_object)
{
    XAML_DECL_VTBL(balls_map, BALLS_MAP_VTBL);
};

EXTERN_C xaml_result XAML_CALL balls_map_new(balls_map**) XAML_NOEXCEPT;

typedef struct balls_ball_score_changed_args balls_ball_score_changed_args;

struct balls_ball_score_changed_args
{
    XAML_STD int32_t ball_num;
    XAML_STD int32_t remain_ball_num;
    XAML_STD uint64_t score;
};

XAML_TYPE(balls_ball_score_changed_args, { 0xa813cb25, 0x86fe, 0x4437, { 0xa6, 0x48, 0xe9, 0xe5, 0x59, 0x79, 0x8d, 0x1f } })

#endif // !BALLS_H
