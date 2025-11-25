-- title:   game title
-- author:  game developer, email, etc.
-- desc:    short description
-- site:    website link
-- license: MIT License (change this to your license of choice)
-- version: 0.1
-- script:  lua

-- Table with information about each button: ID, Name, and default Keyboard key.
local buttons = {
  {id=0, name="UP",    key="UP"},
  {id=1, name="DOWN",  key="DOWN"},
  {id=2, name="LEFT",  key="LEFT"},
  {id=3, name="RIGHT", key="RIGHT"},
  {id=4, name="A",     key="Z"},
  {id=5, name="B",     key="X"},
  {id=6, name="X",     key="A"},
  {id=7, name="Y",     key="S"},
  
  -- Extended Buttons
  {id=8,  name="START",  key="F"},
  {id=9,  name="SELECT", key="D"},
  {id=10, name="L1/LB",  key="E"},
  {id=11, name="R1/RB",  key="W"},
  {id=12, name="L2/LT",  key="R"},
  {id=13, name="R2/RT",  key="Q"},
  {id=14, name="GUIDE",  key="G"},
}

-- Function to draw the status of a single button
function draw_button_status(x, y, button_info)
  local name = button_info.name
  local key = button_info.key
  local id = button_info.id
  
  -- Check if the button is currently pressed
  local is_pressed = btn(id)
  
  -- Set colors based on whether the button is pressed or not
  local box_color = is_pressed and 13 or 14 -- light grey / dark grey
  local text_color = is_pressed and 0 or 12 -- black / white
  
  -- Draw the container box
  rect(x, y, 110, 14, box_color)
  
  -- Draw the button name and its assigned key
  local display_text = string.format("%-7s (Key: %s)", name, key)
  print(display_text, x + 5, y + 4, text_color, false, 1)
end

-- TIC function, called 60 times per second
function ULI()
  -- Clear the screen with a dark blue color
  cls(8)
  
  -- Display title
  print("Enhanced Controller Test", 55, 5, 12)
  
  -- Initial position for drawing the button list (moved up)
  local start_x = 10
  local start_y = 20
  local col_width = 120
  
  -- Loop through all buttons and draw their status
  for i, button_data in ipairs(buttons) do
    local x, y
    
    -- Arrange in two columns
    if i <= 8 then
      -- First column
      x = start_x
      y = start_y + (i-1) * 15
    else
      -- Second column
      x = start_x + col_width
      y = start_y + (i-9) * 15
    end
    
    draw_button_status(x, y, button_data)
  end
end
-- <TILES>
-- 001:eccccccccc888888caaaaaaaca888888cacccccccacc0ccccacc0ccccacc0ccc
-- 002:ccccceee8888cceeaaaa0cee888a0ceeccca0ccc0cca0c0c0cca0c0c0cca0c0c
-- 003:eccccccccc888888caaaaaaaca888888cacccccccacccccccacc0ccccacc0ccc
-- 004:ccccceee8888cceeaaaa0cee888a0ceeccca0cccccca0c0c0cca0c0c0cca0c0c
-- 017:cacccccccaaaaaaacaaacaaacaaaaccccaaaaaaac8888888cc000cccecccccec
-- 018:ccca00ccaaaa0ccecaaa0ceeaaaa0ceeaaaa0cee8888ccee000cceeecccceeee
-- 019:cacccccccaaaaaaacaaacaaacaaaaccccaaaaaaac8888888cc000cccecccccec
-- 020:ccca00ccaaaa0ccecaaa0ceeaaaa0ceeaaaa0cee8888ccee000cceeecccceeee
-- </TILES>

-- <WAVES>
-- 000:00000000ffffffff00000000ffffffff
-- 001:0123456789abcdeffedcba9876543210
-- 002:0123456789abcdef0123456789abcdef
-- </WAVES>

-- <SFX>
-- 000:000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000304000000000
-- </SFX>

-- <TRACKS>
-- 000:100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
-- </TRACKS>

-- <PALETTE>
-- 000:1a1c2c5d275db13e53ef7d57ffcd75a7f07038b76425717929366f3b5dc941a6f673eff7f4f4f494b0c2566c86333c57
-- </PALETTE>

