file(GLOB_RECURSE src_files "*.c*")
idf_component_register(SRCS "ezlopi_cloud_device_types_str.c" "ezlopi_cloud_subcategory_str.c" "ezlopi_cloud_category_str.c" "ezlopi_cloud_item_name_str.c" "ezlopi_cloud_keywords.c" "${src_files}"
    INCLUDE_DIRS "."
    REQUIRES )