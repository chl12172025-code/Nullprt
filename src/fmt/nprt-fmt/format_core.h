#pragma once

void fmt_normalize_indentation(char* text, int* detected_tab_indent);
void fmt_apply_operator_spacing(char* text);
void fmt_wrap_long_lines(char* text, int max_width);
void fmt_force_control_flow_braces(char* text);
