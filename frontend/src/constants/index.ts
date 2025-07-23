export const initialBoilerplate = `# ------------ TEXT SEGMENT ------------ #
add x5,x1,x2 # 0110011-000-0000000-00101-00001-00010-NULL
and x6,x3,x4 # 0110011-111-0000000-00110-00011-00100-NULL
or x7,x5,x6 # 0110011-110-0000000-00111-00101-00110-NULL`

export const REGISTER_ABI_NAMES = [
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0/fp", "s1",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6",
]

export const ITEMS_PER_PAGE = 10;

export const StageColors: Record<string, string> = {
    "FETCH": "#60A5FA",
    "DECODE": "#34D399",
    "EXECUTE": "#F59E0B",
    "MEMORY": "#EC4899",
    "WRITEBACK": "#8B5CF6",
};

export const enum Stage {
    FETCH = 0,
    DECODE = 1,
    EXECUTE = 2,
    MEMORY = 3,
    WRITEBACK = 4
}

export const StageNames = {
    [Stage.FETCH]: 'FETCH',
    [Stage.DECODE]: 'DECODE',
    [Stage.EXECUTE]: 'EXECUTE',
    [Stage.MEMORY]: 'MEMORY',
    [Stage.WRITEBACK]: 'WRITEBACK'
};