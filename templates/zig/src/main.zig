const uli = @import("uli78.zig");

const TICGuy = struct {
    x: i32 = 96,
    y: i32 = 24,
};

var t: u16 = 0;
var mascot: TICGuy = .{};

export fn BOOT() void {}

export fn ULI() void {
    if (uli.btn(0)) {
        mascot.y -= 1;
    }
    if (uli.btn(1)) {
        mascot.y += 1;
    }
    if (uli.btn(2)) {
        mascot.x -= 1;
    }
    if (uli.btn(3)) {
        mascot.x += 1;
    }

    uli.cls(13);
    uli.spr(@as(i32, 1 + t % 60 / 30 * 2), mascot.x, mascot.y, .{ .w = 2, .h = 2, .transparent = &.{14}, .scale = 3 });
    _ = uli.print("HELLO WORLD!", 84, 84, .{});

    t += 1;
}

export fn BDR() void {}

export fn OVR() void {}
