#ifndef CPU_LOGGING_H
#define CPU_LOGGING_H

#define cpu_log_absolute_write    cpu_log_absolute
#define cpu_log_absolute_x_modify cpu_log_absolute_x
#define cpu_log_absolute_x_write  cpu_log_absolute_x
#define cpu_log_absolute_y_write  cpu_log_absolute_y
#define cpu_log_indirect_x_write  cpu_log_indirect_x
#define cpu_log_zero_page_write   cpu_log_zero_page
#define cpu_log_zero_page_x_write cpu_log_zero_page_x
#define cpu_log_zero_page_y_write cpu_log_zero_page_y

typedef void (*Function)(void);

void cpu_log_set_function(byte opcode, Function function);
void cpu_log_set_name(byte opcode, char *name);

void cpu_log_absolute(void);
void cpu_log_absolute_jump(void);
void cpu_log_absolute_x(void);
void cpu_log_absolute_y(void);
void cpu_log_accumulator(void);
void cpu_log_immediate(void);
void cpu_log_implied(void);
void cpu_log_indirect(void);
void cpu_log_indirect_x(void);
void cpu_log_indirect_y(void);
void cpu_log_relative(void);
void cpu_log_zero_page(void);
void cpu_log_zero_page_x(void);
void cpu_log_zero_page_y(void);

void cpu_log_operation(void);

#endif /* CPU_LOGGING_H */
