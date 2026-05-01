import math
import pygame

# ---------------------------------------------------------
# HELPER FUNCTIONS
# ---------------------------------------------------------
def clamp(val, min_val, max_val):
    return max(min_val, min(max_val, val))

def forward_kinematics(base_pos, angles, lengths):
    x, y = base_pos
    t1, t2 = angles
    l1, l2 = lengths
    
    # Joint 1 (Elbow)
    j1_x = x + l1 * math.cos(t1)
    j1_y = y + l1 * math.sin(t1)
    
    # End Effector
    ee_x = j1_x + l2 * math.cos(t1 + t2)
    ee_y = j1_y + l2 * math.sin(t1 + t2)

    return (j1_x, j1_y), (ee_x, ee_y)

def inverse_kinematics(target_pos, base_pos, lengths):

    l1, l2 = lengths
    # Target relative to base
    dx = target_pos[0] - base_pos[0]
    dy = target_pos[1] - base_pos[1]
    dist = math.hypot(dx, dy)

    # Check reachability
    if dist > (l1 + l2) or dist == 0:
        return None

    # Law of Cosines for Elbow (theta 2)
    # dist^2 = l1^2 + l2^2 - 2*l1*l2*cos(angle_C)
    # We want external angle t2, so we use slightly different trig:
    cos_angle_2 = (dx**2 + dy**2 - l1**2 - l2**2) / (2 * l1 * l2)

    # Floating point clamp to prevent math domain error
    cos_angle_2 = clamp(cos_angle_2, -1.0, 1.0)

    # Elbow angle (relative to forearm)
    t2 = math.acos(cos_angle_2)

    # Shoulder angle (theta 1)
    # t1 = atan2(y, x) - atan2(l2*sin(t2), l1 + l2*cos(t2))
    k1 = l1 + l2 * math.cos(t2)
    k2 = l2 * math.sin(t2)
    t1 = math.atan2(dy, dx) - math.atan2(k2, k1)

    return t1, t2

# ---------------------------------------------------------
# MAIN SIMULATION
# ---------------------------------------------------------
def main():
    pygame.init()

    # 1. VISUAL SETUP (Fixed Cutoff Issue)
    W, H = 960, 800  # Increased Height to fit the arm
    screen = pygame.display.set_mode((W, H))
    pygame.display.set_caption("2D Arm Simulation | Manual & IK Control")
    clock = pygame.time.Clock()

    # Colors (Engineering Theme)
    C_BG = (240, 240, 245)      # Blueprint White
    C_GRID = (200, 200, 210)    # Light Grid
    C_LINK = (50, 60, 70)       # Dark Steel
    C_JOINT = (0, 120, 215)     # Blueprint Blue
    C_TRAIL = (200, 50, 50)     # Red Trail
    C_TEXT = (40, 40, 50)

    # Arm Definition
    base = (W // 2, H // 2)     # Centered Base
    l1, l2 = 190, 150           # Link lengths
    lengths = (l1, l2)
    reach = l1 + l2

    # State Variables
    t1 = -math.pi / 2  # Start pointing up
    t2 = 0.1
    w1, w2 = 0.0, 0.0  # Velocities

    # Physics Parameters
    ACCEL_POWER = 5.0  # Manual torque
    DAMPING = 0.92     # Friction (0.0 = no friction, 1.0 = stuck)

    # Control Mode
    follow_mode = False  # Toggle for "Exploration" feature
    
    # PD Controller Gains (for Follow Mode)
    Kp = 150.0  # Proportional (Spring force)
    Kd = 10.0   # Derivative (Damping to stop oscillation)

    trail = []
    font = pygame.font.SysFont("Consolas", 18)

    running = True
    while running:
        dt = clock.tick(60) / 1000.0  # Seconds

        # --- INPUT HANDLING ---
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE: running = False
                if event.key == pygame.K_f:      follow_mode = not follow_mode # Toggle Feature
                if event.key == pygame.K_c:      trail.clear() # Clear trail

        # --- PHYSICS & CONTROL LOGIC ---
        a1, a2 = 0.0, 0.0 # Angular acceleration

        mx, my = pygame.mouse.get_pos()

        if follow_mode:
            # --- EXPLORATION FEATURE: INVERSE KINEMATICS + PD CONTROL ---
            # 1. Calculate ideal angles to hit mouse
            target_angles = inverse_kinematics((mx, my), base, lengths)

            if target_angles:
                tgt_t1, tgt_t2 = target_angles

                # 2. PD Control: Apply acceleration to move towards target angles
                # Torque = Kp * error - Kd * velocity
                error1 = tgt_t1 - t1
                error2 = tgt_t2 - t2

                # Unwrap angles so it doesn't spin 360 unnecessarily
                error1 = (error1 + math.pi) % (2 * math.pi) - math.pi
                error2 = (error2 + math.pi) % (2 * math.pi) - math.pi

                a1 = (Kp * error1) - (Kd * w1)
                a2 = (Kp * error2) - (Kd * w2)
        else:
            # --- MANUAL CONTROL ---
            keys = pygame.key.get_pressed()
            if keys[pygame.K_q]: a1 -= ACCEL_POWER
            if keys[pygame.K_e]: a1 += ACCEL_POWER
            if keys[pygame.K_a]: a2 -= ACCEL_POWER
            if keys[pygame.K_d]: a2 += ACCEL_POWER

            # Apply natural damping (friction) in manual mode
            a1 -= w1 * (1.0 - DAMPING) * 10
            a2 -= w2 * (1.0 - DAMPING) * 10

        # Integration (Euler)
        w1 += a1 * dt
        w2 += a2 * dt
        t1 += w1 * dt
        t2 += w2 * dt

        # --- DRAWING ---
        screen.fill(C_BG)

        # Draw Grid
        for i in range(0, W, 50):
            pygame.draw.line(screen, C_GRID, (i, 0), (i, H))
        for i in range(0, H, 50):
            pygame.draw.line(screen, C_GRID, (0, i), (W, i))

        # Draw Workspace Boundary
        pygame.draw.circle(screen, (220, 220, 230), base, reach, 2)

        # Forward Kinematics for drawing
        j1, ee = forward_kinematics(base, (t1, t2), lengths)

        # Trail
        if len(trail) == 0 or math.hypot(trail[-1][0] - ee[0], trail[-1][1] - ee[1]) > 5:
            trail.append(ee)
        if len(trail) > 100: trail.pop(0)
        if len(trail) > 1:
            pygame.draw.lines(screen, C_TRAIL, False, trail, 2)

        # Draw Arm (Thick Lines)
        pygame.draw.line(screen, C_LINK, base, j1, 8)
        pygame.draw.line(screen, C_LINK, j1, ee, 6)

        # Draw Joints
        pygame.draw.circle(screen, C_JOINT, base, 12)
        pygame.draw.circle(screen, C_JOINT, j1, 10)
        pygame.draw.circle(screen, (50, 200, 50) if follow_mode else (200, 100, 50), ee, 8)

        # --- HUD (HEADS UP DISPLAY) ---
        ui_texts = [
            f"MODE: {'AUTO (Follow Mouse)' if follow_mode else 'MANUAL (Q/E, A/D)'}",
            f"FPS:  {clock.get_fps():.0f}",
            f"T1: {math.degrees(t1):.1f}°  T2: {math.degrees(t2):.1f}°",
            "Controls: [F] Toggle Mode | [C] Clear Trail"
        ]

        for i, txt in enumerate(ui_texts):
            surf = font.render(txt, True, C_TEXT)
            screen.blit(surf, (10, 10 + i * 25))

        pygame.display.flip()

    pygame.quit()

if __name__ == "__main__":
    main()