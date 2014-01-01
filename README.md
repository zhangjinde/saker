# Saker
===========================================
## Interface for lua
--------------------
  ### saker.sysinfo , the parmary key below. 
  ~
  Eg: ctx,err = saker.sysinfo(system.boottime)
        if err ~= nil then
            return False,err
        end
        print(ctx)
  ~ 
  
    #### system.boottime                                                          
    #### system.uptime                      
                                      
    #### system.cpu.num.online              
    #### system.cpu.num.max                 
    #### system.cpu.load.all                
    #### system.cpu.util                    
                                        
    #### system.mem.total                   
    #### system.mem.free                    
    #### system.mem.buffers                 
    #### system.mem.cached                  
    #### system.mem.used                    
    #### system.mem.pused                   
    #### system.mem.available               
    #### system.mem.pavailable              
    #### system.mem.shared                  
                                       
    #### system.net.if.in                   
    #### system.net.if.out                  
    #### system.net.if.total                
    #### system.net.if.collisions           
                                      
    #### system.kernel.maxfiles             
    #### system.kernel.maxproc              
                                     
    #### system.fs.size.total               
    #### system.fs.size.free                
    #### system.fs.size.used                
    #### system.fs.size.pfree               
    #### system.fs.size.pused               
                                      
    #### system.proc.id                     
    #### system.proc.mem.used               
    #### system.proc.mem.pused              
    #### system.proc.cpu.load               
    #### system.proc.all                    
       
* 

## Get, and Compile ,install
------------------------------
  1. git clone git://github.com/cinience/Saker.git
  2. compilte
  * if you use vs2010 , please use build/msvs/Saker.sln *
	* if you use gcc    , please install cmake and sh build.sh *
  3. install
  * if you use windows, please use tools/Admin.bat *
  * if you use *nix   , please use tools/Admin.sh  *
  
## Developing
------------------------------
  if you want join me ,please email cinience#qq.com 
  
## Plan
------------------------------
  consummate sysinfo 
  consummate autorun in linux 
  add example and unittest

