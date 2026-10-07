-- Modified 2026-10 by Anthony Airdo for the web build (gltron-web).
-- dump global environment

function savevar (n,v)
 if v == nil then return end
 if type(v)=="userdata" or type(v)=="function" then return end
 -- if type(v)=="userdata" or type(v)=="function" then write("\t-- ") end
 -- don't print lua constants
 if strsub(n, 1, 1) == "_" then return end
 write("settings.", n," = ")
 if type(v) == "string" then write(format("%q",v))
 elseif type(v) == "table" then
   if v.__visited__ ~= nil then
     write(v.__visited__)
   else
    write("{ }\n")
    v.__visited__ = n
    for r,f in v do
      if r ~= "__visited__" then
        if type(r) == 'string' then
          savevar(n.."."..r,f)
	else
          savevar(n.."["..r.."]",f)
	end
      end
    end
   end
 else write(tostring(v)) end
 write("\n")
end

-- savevar marks every table it writes (__visited__) to catch loops. Clear
-- the marks afterwards: otherwise the next save in the same run writes
-- "settings.keys = keys" and the key table is gone on the next start.
function unmark (n,v)
 if type(v) == "table" and v.__visited__ ~= nil then
   v.__visited__ = nil
   foreach(v,unmark)
 end
end

function save ()
  foreach(settings,savevar)
  foreach(settings,unmark)
end
