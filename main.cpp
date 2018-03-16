/***
 * @SmartWater
 * @version                 1.5.1 [Candidato]
 * @author                  Rolando Arriaza/rolignu90 (senior backend - frontend software desing )
 * @description             Core del firmware smartwater
 *
 * 
 * Version 1.5.1  :  
 * 
 *              -- se agregaron parches de seguridad y mejor conectividad con la nube 
 *              -- se creo el parse json de acuerdo a sus variables a excepcion de las v VIRTUALES
 *              -- se arreglo un bug en el temporizador principal 
 * 
 * Descripcion de los servicios
 *
 *  EBASE       = envio de data al web services : se encarga de establecer la data y la logica dentro del WS
 *  AERROR      = envio de errores de parte del dispositivo al webservices
 *  ECONTROL    = Gestor de control del dispositivos , en el cual se ve estados en tiempo real
 *  WAR         = de parte del cliente o HTTP envia comandos de configuracion
 * 
 * 
****/



/**
 * 
 * Nota : las funciones particle se limitan a envio de datos maximo 63 caracteres :S
 * 
 * CONFIG STRING : --->  JCONF;{"version":"1","webservices":"hook-response/Ebase","web_base":"Ebase","variables":"{}","activate":"1","sleep":"0","tick":"60000","war":"war"}
   
   Ejemplo de envio de variables :
   
   VAR;{"n":"var0","p":"0","t":"A","f":"D","a":"f","v":"0"}
   
   
   EJECUCIONES EN EL WAR :
   
        TEST_VAR;{name}    --- verifica si una variable esta cargada en el sistema
        
        FORCE_VAR;{null}   --- el sistema volvera a cargar las variables o forzara ha hacerlo 
        
        PARTICLE;{funtion} 
		
		        ACTIVATE  --- VERIFICA SI EL PHOTON ESTA CONECTADO 
		        RESET     --- ENVIA UNA SEÑAL DE RESET AL PHOTON 
		        SAFE_MODE --- ENTRA EN MODO SEGURO 
		

        VAR;{json}                  -- CONFIGURA UNA NUEVA VARIABLE EN FORMATO JSON TANTO COMO EN LA EEPROM Y EN EL OBJETO 
        
                Ejemplo : VAR;{"n":"var0","p":"0","t":"A","f":"D","a":"f"}
                
                        n       = nombre 
                        p       = pin 
                        t       = tipo 
                        f       = formato 
                        a       = activo 


        VERSION;null                --- verifica en que version esta el algoritmo 

        JCONF; {JSON} (DEPRECADO )  --- AGREGA CONFIGURACIONES EN JSON , SE DEPRECO POR FALTA DE MEMORIA 
        
                    --tomar en cuenta que en si no esta del todo deprecada sus funcionamiento es limitado nada mas 
                    
                      {
                            "version":"1",
                            "webservices":"hook-response/Ebase",
                            "web_base":"Ebase",
                            "variables":"{}",
                            "activate":"1",
                            "sleep":"0",
                            "tick":"60000",
                            "war":"war"
                        }
                        
                    --se puede realizar una mejora de funcionalidad pero queda a critero futuro 

        CONF_RESET;null             --- RESTABLECE LA CONFIGURACION POR DEFECTO 

        VAR_VIRTUAL;null            ---VERIFICA SI EL OBJETO VIRTUAL ESTA CARGADO EN MEMORIA 

        VAR_CONF;null               --- VERIFICA SI EL OBJETO DE CONFIGURACION O OBJSETUP NO ESTA VACIO 
        
        DELETE_VAR;{name}           --- ELIMINA UNA VARIABLE POR MEDIO DE SU NOMBRE 
        
                EJEMPLO : DELETE_VAR;var9  -- donde var9 es el nombre asignado 
                
                
        EMERGENCY COMMANDS OR EMER :
        
                    1 = REINICIAR SERVICIOS 
                    2 = BORRAR EEPROM 
                    3 = AGREGAR CONFIGURACION POR DEFECTO A LA EEPROM 
                
                
    LIMITACIONES :
    
    
                -- LAS VARIABLES OUTPUT DE TIPO ANALOGAS SE LIMITAN A VALORES ENTEROS , SE PUEDE CREAR UN PARCHE GENERANDO VALORES FLOTANTES 
                        --   PARA CREAR ESTE PARCHE SE NECESITA MANIPULAR  LA ESTRUCTURA VarObject DENTRO DE Smartobject.cpp
                        --   LUEGO CAMBIAR LAS FUNCIONES DE ENTERO A FLOAT O DOUBLE 
                        --   HACER LA CONVERSION DE CHAR* TO FLOAT O DOUBLE 
                        --   LISTO.
                        
                        
                -- LIMITACION DE MEMORIA : LA MEMORIA EEPROM ES MUY PEQUEÑA ASI QUE 
                                           SOLO NOS PERMITA ALMACERNAR UNA PEQUEÑA CADENA DE APROX 600 CARACTERES
                                           ASI QUE LA VARIABLE VIRTUAL TIENE UN MAXIMO DE 15 OBJETOS 
                                           
                                           SOLUCION : CAMBIAR LA EEPROM POR UNA SD Y OTRO DISPOSITIVO DE ALMACENAMIENTO 
                                                      SI SE CAMBIAN TENER EN CUENTA LAS FUNCIONES DE LECTURA Y ESCRITURA 
                                                      EL FIRMWARE
                                                      
                                           CONSEJO  : LA SOLUCION ES UN POCO COMPLEJA ASI QUE SI NO PUEDE C O C++ NO INTENTEN  
 *             
 *              -- CLOUD MAX LENGHT :  LA NUBE DE PARTICLE TIENE UN LIMITANTE DE APROX 500 CARACTERES QUE SE ENVIAN Y RECIBEN 
 *                                      
 *                                      SOLUCION : NO USAR LA NUBE DE PARTICLE 
 *              
 *              -- VARIABLES CUYA MEMORIA ES COMPARTIDA :
 * 
 *                              -- CUANDO UNA VARIABLE SE COMPORTA DE FORMA INUSUAL EN SU MAYORIA ES PORQUE EL PUNTERO 
 *                              -- DIRECCIONO EN UN SEGMENTO DE MEMORIA COMPARTIDA, ESO LO HACE PORQUE SE HA TOPADO LA 
 *                              -- MEMORIA RAM , ESO PROVOCA ERRORES DE PARSEO 
 * 
 *  
 *                              SOLUCION : REINICIAR EL PHOTON (AGREGAR MAS MEMORIA RAM :D)
 * 
 * 
***/


#include <SmartObject.cpp>
#include <ParseVariable.cpp>





/*****************************  SMART WATER INICIO E INSTANCIA DE LA CLASE  *******************/



/***
 * @description : Esta clase fue desarrollada con el fin de hacer una ejecucion en segundo plano
 *                a pesar de que esta la de primer plano se ejecuta asincronamente.
 * @version : 1.1
 * @author : Rolando Arriaza
***/
class Task
{
    public :


              Task() :  timer(this->tick , &Task::task_process , *this) {
                   //this->parent = s;
              }
               ~Task() { this->task_stop(); }
               void task_process(void);
               void task_start()  { timer.start();  }
               void task_stop(){ timer.stop(); }
               void new_period(int t){
                   this->tick = t;
                   timer.changePeriod(this->tick);
               }
               void reset_task() { timer.reset();   }
               

    private:
             int tick       = 10000;
             Timer timer;
};




class Swater : public Task , public SmartVariables
{

    public:

             /**
             * @description  Constructor de la clase hace referencia al void setup ()
             *               este constructor carga todas las funciones que se desean utilizar
             * @version 1.4.0
             * @author Rolando Arriaza
             * 
             *  parche :  
             *          1.4 ->  al momento de iniciar el constructor por primera ve no la EEPROM no tenia configurado el objeto 
             *                  entonces se agrego si l la version del objeto es menor a cero significa que la eprom no se ha quemado 
             *                  por primera vez , se cargara una configuracion por defecto a la eeprom , esta configuracion es estatica
             *                  este error se soluciono en el contructor de Swater.
             * 
            **/
            Swater() : timer(this->tick , &Swater::loop , *this) , Task() {


                 //obtiene el objeto desde la EEPROM
                 this->ObDefault_  = this->get_PrimaryConf();
                 
                 //parche 1.4 
                 if(this->ObDefault_.version <= 0 ){
                     
                     ObjSetup o ;
                     o = DefConfig::get_DefConf();
                     this->set_PrimaryConf(o);
                     this->ObDefault_ = o ;
                 }
                 

                 //callbacks de particle

                 //generacion de datos en consola
                 Particle.variable("console" , console_);

                 //generacion de errores
                 Particle.variable("error" ,  error_);
                 
                 
                 //funcion de emergencia por si algo sale mal en la configuracion 
                 //o en otras instancias..
                 Particle.function("EMER", &Swater::emergencyConfig , this);
                 
                 //funcion de prueba , en cual ahi hay fragmentos de codigos 
                 //de tipo relativo , y con respuestas varias
                 Particle.function("TESTING" , &Swater::TestFunction , this);
                 

                 //instancia el ultimo tick configurado en milisegundos
                 if(this->tick != ObDefault_.tick ){
                     this->tick =  ObDefault_.tick ;
                 }
                 
                 
                 //funcion que a object var le da una localidad de memoria 
                 this->TruncateVariables();
                 
                 //obtiene el id del dispositivo 
                 this->deviceId = System.deviceID();

            }



             /**
             * @description  inicia los objetos despues del constructor
             * @version 1.0
             * @author Rolando Arriaza
             * @param Tstart boolean , verifica si se ejecuta el timer
            **/
            void init(boolean Tstart)   {


                //tick = tiempo en milisegundo que se ejecutara un loop


                //testmode se utiliza la variable callback con el resultado dentro de json_compose
                if(test_mode){
                    Particle.variable("callback" , json_compose);
                }


                //regresa la configuraciond e memoria a modo default
                if(default_mode)
                {

                    ObjSetup o = DefConfig::get_DefConf();
                    this->ObDefault_ = o ;
                }


                ObjSetup setup = this->ObDefault_;
                
                


                //nombre del webhook donde esta alojado el WS
                 web_service = String(setup.webservices);

                 //web base
                 web_base = String(setup.web_base);

                 //web services error
                 web_service_error = String(setup.error);

                 //hook name error
                 web_base_error = String(setup.base_error);

                 //control_service
                 control_service = String(setup.control_service);

                 //control del hook
                 web_control     = String(setup.control_base);

                 //war function services
                 web_config      = String(setup.war);

                //funcion de particle en la cual se podra enviar las configuraciones WAR
                Particle.function(web_config, &Swater::set_config , this);


                //llamada hacia el webhook , funcion por defecto Gettrigget
                Particle.subscribe(this->web_service,  &Swater::GetTrigger  , this);


                //llamada hace el webhook error data handler
                Particle.subscribe(this->web_service_error , &Swater::GetErrorHandle , this);


                //llamada o suscripcion al servidor de control
                Particle.subscribe(this->control_service, &Swater::GetControlHandle , this );


               
                 //verifica el status del firmware ...
                 this->FirmStatus();
                 
                  
                 
                 
                 //verificamos si inicializa las variables 
                 //en dado caso nos devuelve falso la condicion 
                 //booleana que detectara en el ciclo se coloca como falso 
                 this->InitVariables(true);
                
                 if(!this->InitVariables(false) )
                 {
                    //verifica si se ejecuto una variable en dado caso se halla ejecutado entonces 
                    //no hay necesidad de volver a ejecutar en la instancia del loop 
                    this->ExecVar = false;
                 }
                 
                


                //instanciamos el timer dentro de init
                 if(Tstart){

                   /* if(this->tick != ObDefault_.tick ){
                        this->tick =  ObDefault_.tick ;
                        set_period(this->tick);
                    }*/
                    
                    if(this->tick != setup.tick ){
                        this->tick =  setup.tick ;
                        set_period(this->tick);
                    }

                    timer.start();

                 }


                 if(activate_task)
                    this->task_start();

        

            }



             /**
             * @description  destructor de la clase
             * @version 1.0
             * @author Rolando Arriaza
            **/
            ~Swater(){}



             /**
             * @description  desarrolla un loop controlado
             * @version 1.0
             * @author Rolando Arriaza
            **/
            void loop(void);




            /**
             * @description  verifica el estado del firmware
             * @version 1.0
             * @author Rolando Arriaza
            **/
            int FirmStatus();


            /**
             * @description  Proceso en el cual obtiene los valores que enviara el photon
             * @version 1.0
             * @author Rolando Arriaza
            **/
            String get_process();



            /******************************FUNCIONES VARIAS**************************************/


            /**
             * @description  activa el modo test esta ligada a la variable particle callback
             * @version 1.0
             * @author Rolando Arriaza
             * @params boolean test_mode
            **/
            void start_testMode(boolean t){ test_mode = t; }




             /**
             * @description  activa la configuracion general en modo defualt
             * @version 1.0
             * @author Rolando Arriaza
             * @params boolean d = true , borra la configuracion anterior
            **/
            void start_defaultMode(boolean d) { default_mode = d; }



            /**
             * @description establecemos el periodo de la ejecucion loop
             * @version 1.0
             * @author Rolando Arriaza
             * @params int t = tiempo en milisegundos
            **/
            void set_period(int t) {
                tick = t;
                timer.changePeriod(tick);
            }


            /**
             * @description  se establece el tiempo tick pero este no afecta el periodo de ejecucion
             * @version 1.0
             * @author Rolando Arriaza
             * @params int t = tick en milisegundos
            **/

            void set_tick (int t) { tick = t; }




            /**
             * @description detiene el ciclo de ejecucion del loop
             * @version 1.0
             * @author Rolando Arriaza
            **/

            void stop_timer(){ timer.stop(); }


             /**
             * @description establece la configuracion del sistema en el WAR
             * @version 1.0
             * @author Rolando Arriaza
            **/
            int set_config(String command);
            
            
              /**
             * @description establece la configuracion de emergencia 
             * @version 1.0
             * @author Rolando Arriaza
            **/
            int emergencyConfig(String command);


          /**
             * @description establece la configuracion de emergencia 
             * @version 1.0
             * @author Rolando Arriaza
            **/
            int TestFunction(String command);

           

           /**
             * @description clase de la tarea asincrona
             * @version 1.5
             * @author Rolando Arriaza
            **/
           // Task task;
           
           
           bool ProgramConf(char * new_data ){
                
           
                
                //obtenemos la configuracion primaria 
                 ObjSetup o = this->get_PrimaryConf();
                 

                 //variables de tipo O 
                // String vars =  o.variables;
                char * vars = NULL;
                vars        = o.variables;
                 
                 
                if(vars == NULL || vars == "" || o.variables == '\0'){
                        strcpy(o.variables , new_data);
                }
                else{

                        char* pivot          = this->SubVarC( new_data , o.variables );
                        sprintf(console_ , "%s" , pivot );
                        strcpy(o.variables , pivot);
                }
                
                
                 //log de ovariable para pruebas 
                 //sprintf(error_ , "%s" , strlen(o.variables) );
                 
                 this->ObDefault_ = o ;
                //mandamos a guardar la configuracion luego de la actuacion cola 
                 set_PrimaryConf(o);
                 
                 
                 //Log de consola para pruebas 
                 //sprintf(console_ , "%s %s" , "Proceso [Ok] " , new_data);
                
                     
                return true ;
           }
          
           
          /**
           * @Author Rolando Arriaza 
           * @version 1.9
           * @params char* cola de datos
           * @params char * variables disponibles
           * @mejoras : 
           *            
           *            -- mayor velocidad en las secuencia de datos 
           *            -- mayor fiabilidad usando strtok_r 
           *            -- se creo un espacio en memoria en la variable result
          ****/ 
          
          char * SubVarC(char * queue, char variables[]  ){
               
                char * result;  										  //declaramos la variable de resultado
                
					
                int len = 6666;										     //agregamos la cantidad de memoria que puede utilizar esa variable
				result = (char*) malloc((len + 1) * sizeof(char));      //creamos la memoria volatil
				memset(result , 0 , len);                              // agregamos ceros "0" a la localidad creada 
                
                
                //patron 
                char * pattern    = " ;";
                
                //variable de la cadena de contencion 
				char * str;
               
           
                //variable de prueba .. sustituir por JSON result
                char * q_index = this->ConfJson(queue , "n");
                
          
                
                //el algoritmo debe de cumplir con siertas condiciones para llegar a un estado natural 
                //en dado caso no cumpla ese estado es alterado por el nuevo parametro 
                bool q_exist 		= false;
                
                //verificamos iteracion de datos por iteracion 
                while (str  = strtok_r(variables, pattern, &variables)){
                	
                	char *v_name      = this->ConfJson(str , "n");

					//primera comparacion de datos , verificar si la cola es igual al primer valor de la iteracion 
                	if(strcmp(q_index,v_name) == 0){
                		
                		/**
                		   Si es igual se verifica si el resultado esta vacio o ceros 
                		   si en dado caso esta vacio se copia en memoria en dado caso no 
                		   esta vacio se concatena los datos 
						**/
						
						if(result[0] == '\0'){
                			strcpy(result , queue);
						}
						else{
							strcat(result , ";");
							strcat(result , queue);
						}
						
						/**
						   si existe en la cola y se remplaza entonces 
						   la condicion existente sera cierta en cualquiera de las 
						   demas iteraciones
						***/
						q_exist 		= true;
						
					}
					else {

						// en dado caso no aparezca entonces se agrega el valor actual de la variable
					   	if(result[0] == '\0'){
                			strcpy(result , str);
						}
						else{
							strcat(result , ";");
							strcat(result , str);
						}
					   
					}

		
                	
				}
				
				//si no existio un reemplazo entonces se agrega el valor 
				//que se desea guardar al final de la cola 
				if( !q_exist ){
					
					   	if(result[0] == '\0'){
                			strcpy(result , queue);
						}
						else{
							strcat(result , ";");
							strcat(result , queue);
						}
					   
				}
                
                
                return  result;
                
        }
         
           
          

            /**
             * Funciones para la configuracion inicial de los servidores
             * utilizando la memoria EEPROM por defecto.
            ***/


            /**
             * @description ejecuta una nueva escritura de configuracion en la EEPROM
             * @version 1.1
             * @author Rolando Arriaza
            **/
            int set_PrimaryConf(ObjSetup o) {
                
                int size = (EEPROM.length() - 1);
                if(size > 2000)
                {
                    //EEPROM.clear();
                    //delay(1500);
                    EEPROM.put(this->addr_  , o);
                    delay(2000);
                    return 1;
                }
                else
                {
                    sprintf(error_ , "%s" , "Error al Escribir la memoria EEPROM");
                    return 0;
                }

               
                return -1;
            }



              /**
             * @description obtiene la configuracion desde la localidad en la EEPROM
             * @version 1.1
             * @author Rolando Arriaza
            **/
            ObjSetup get_PrimaryConf()
            {
                 int size = (EEPROM.length() - 1);
                 ObjSetup o ;

                 if(size > 2000)
                    EEPROM.get(this->addr_ , o);
                    

                 return o;

            }
            
            
            /**
             * @deprecada v2.0.1
             * 
            ***/
            bool get_version (){
                 ObjSetup o = this->get_PrimaryConf();
                 if(o.version <= 0 ){
                     return false;
                 }
                 return true ;
            }
            
            


             /**
             * @description Avtiva la tarea asincrona
             * @version 1.1
             * @author Rolando Arriaza
            **/
            void start_task(boolean t) { activate_task = t; }



             /**
             * @description  obtiene la cadena de comando segun su indexado
             * @example  PARTICLE;START -->  "COMMANDO"  ";" "VALOR"
             * @version 1.1
             * @author Rolando Arriaza
            **/
            String SplitCommand(String data, char separator, int index)
            {
                int found = 0;
                int strIndex[] = { 0, -1 };
                int maxIndex = data.length() - 1;

                 for (int i = 0; i <= maxIndex && found <= index; i++) {
                    if (data.charAt(i) == separator || i == maxIndex) {
                        found++;
                        strIndex[0] = strIndex[1] + 1;
                        strIndex[1] = (i == maxIndex) ? i+1 : i;
                    }
                }

                return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
            }



            char* string2char(String command){
                if(command.length()!=0){
                    char *p = const_cast<char*>(command.c_str());
                    return p;
                }
            }
            
            
            
            char* ConvertoChar(String str){

                int str_len = str.length() + 1;
                char char_array[str_len];
                str.toCharArray(char_array, str_len);

                return char_array;

            }
            
            
            bool creatVariables(char * name , char * pin , char* type , char * format , char * active , char * value ){
                
                
				 int index  = this->TestVar(name , true  );
				 
				 if(index > -1 ){
					 
					 //Esta variable ya esta configurada dentro del objeto varOBJ 
					 // vamos a sobre escribir sus propiedades
					 
					 this->PutVariables(name,pin,type,format,active,index,value);
					 
				 }
				 else {
					 
					  //en dado caso no exista entonces obtenemos el index var 
					  this->PutVariables(name,pin,type,format,active,this->IndexVar,value);
					  this->IndexVar++;
				 }
                 
                 return true;
                 
                
            }
			
			
			int TestVar( const char * name , bool index  ){
				
				
				/**	FUNCION COMANDO PARA VERIFICAR SI LA 
					VARIABLE X ESTA BIEN CONFIGURADA DENTRO DE LA COLA
				***/
       
				int size = sizeof(this->varObj)/sizeof(*this->varObj);
         
				//si el nombre de la variable se encuentra integrada dentro del ciclo 
				//entonce devolvera 1 = true 
   
				if(index == true  ){
					
					for(int i = 0 ; i < size ; i++){
						if(strcmp(this->varObj[i].name , name ) == 0){
								return i;
						}
					}
					
					return -1;
					
				}
				else {
					
					for(int i = 0 ; i < size ; i++){
						if(strcmp(this->varObj[i].name , name ) == 0){
								return 1;
						}
					}
					
					return 0;
				}
				

			}
			
			void PutVariables(char * name , char * pin , char* type , char * format ,char * active , int index , char* value ){
             
              //VAR;{"n":"var0","p":"0","t":"A","f":"D","a":"f"}
			  
              int pin_ = -1;
              varObj[index].name = name;
              
             

              if(pin == "null")
                varObj[index].pin = -1;
              else 
                varObj[index].pin = atoi(pin);
              
			  
			  if(value == "null" )
				  varObj[index].value = 0 ;
			  else 
				  varObj[index].value = atoi(value);
				  
              
              
              if(strcmp(active , "t") == 0)
                    varObj[index].active = true;
              else 
                    varObj[index].active = false;

               /*
                    Nota:
                    
                       existe un comportamiento extraño al momento de 
                       colocar de varObj[index].type = type;
                       al parecer genera caracteres extraños.
                       
                       aclaro use la solucion mas simple ya que no tengo tiempo 
                       de analizar el error asi que una mente menos ocupada podria solucionarlo 
                       
               **/
               
               if(strcmp(type, "AI") == 0){  //analogo de entrada
                     varObj[index].type = "AI";
                     pin_ =  this->convertPIN( varObj[index].pin , 1);
                     pinMode( pin_ , INPUT); 
                     varObj[index].isconfig = true;
               }
               else if(strcmp(type, "AO") == 0){ //analogo de salida
                    varObj[index].type = "AO";
                    pin_ = this->convertPIN( varObj[index].pin , 0);
                    pinMode( pin_  , OUTPUT); 
                    analogWrite( pin_ , 0);
               }
               else if(strcmp(type, "DI") == 0  ){ // digital entrada
                    varObj[index].type = "DI";
                    pin_ = this->convertPIN( varObj[index].pin , 2);
                    pinMode(pin_ , INPUT);
               }
               else if(strcmp(type, "DO") == 0){ //digital de salida 
                    varObj[index].type = "DO";
                    pin_ = this->convertPIN( varObj[index].pin , 2);
                    pinMode( pin_  , OUTPUT); 
                    digitalWrite( pin_ , 0);
               }
               
             
             
               
               
               if(format == "I")
                    varObj[index].format = "int";
               else if(format == "D")
                    varObj[index].format = "double";
               else if(format == "S")
                    varObj[index].format = "string";


               varObj[index].test = "P"; 
               
                 
         }
         
            bool DeleteVariable(const char* name ){
                 
                 bool success = true;

                 // primer paso para eliminacion de una variable 
                 // eliminar esa variable dentro del objeto 
                 // la parte chiche de eliminar xD
                 /*int size       =  sizeof(varObj)/sizeof(*varObj);
                 int i          = 0;
                 for(i  ;  i < size ; i++){
                     if(strcmp(varObj[i].name , name) == 0){
                            //liberamos el objeto 
                            varObj[i].name       = NULL;
		                    varObj[i].type       = NULL;
		                    varObj[i].format     = NULL;
		                    varObj[i].test       = NULL;
		                    varObj[i].isconfig   = false ;
		                    
		                    break;
                     }
                     
                 }*/
               
                 
                 // viene lo yuca , eliminarlo de la EEPROM :S 
                 // esta parte hay que ser cuidadoso ya que se leera el objeto mencion de la eeprom 
                 
                 ObjSetup object = this->ObDefault_;
                 
                 
                 //por seguridad se leera antes el objeto verificando que existe la cadena 
                 if(object.variables == NULL 
                        || object.variables == '\0' 
                        || object.variables[0] == '\0')
                    return false;
                    
                
                 //creamos las variables necesarias para la manipulacion de la data 
                 char * str;
                 char * _name           = NULL;
                 char * variables       = NULL;
                 char * result          = NULL;
                 
                 
                 int len                = 6666;	
                 result                 = (char*) malloc((len + 1) * sizeof(char));  
                
                 memset(result , 0 , len);    
                 
                 
                 variables              = (char*) malloc(( strlen(object.variables) + 1) * sizeof(char));
                 //new_vars               = (char*) malloc(( strlen(object.variables) + 1) * sizeof(char));
                 
                 //copiamos el object.variables a variables 
                 strcpy(variables, object.variables);
                 
                  while( str =  strtok_r(variables , " ;" , &variables )){
                      
                       //obtenemos el nombre :D 
                       _name         =  this->ConfJson(str , "n");
                       
                       if(strcmp(_name , name ) != 0){
                           
                           if(result [0] == '\0' ){
                               sprintf(error_ , "%s" , name );
                			    strcpy(result , str);
						    }
						    else{
						    	strcat(result , ";");
							    strcat(result , str);
						    }
                       }
                      
                  }
                  
                  sprintf(console_ , "%s" , result );
                  strcpy(object.variables , result);
                  
                  this->ObDefault_ = object;
                  set_PrimaryConf(object);
                  
                  
                  free(_name);
                  free(variables);
				  
				  //reiniciamos las variables tipo objeto 
				  this->InitVariables(true);
              
                 
                 return success;
        
             }
            

             void TruncateVariables(){
				 
			   //IndexVar se resetea ya que las variables mueren 
			   this->IndexVar = 0 ;
			
				// obtenemos el tamaño del objeto , generalmente es 15 
               int size       =  sizeof(varObj)/sizeof(*varObj);
             
               for(int i = 0 ; i < size ; i++ ){
		           varObj[i].name       = NULL;
		           varObj[i].type       = NULL;
		           varObj[i].format     = NULL;
		           varObj[i].test       = NULL;
		           varObj[i].value      = 0;
		           varObj[i].isconfig   = false;
	           }
			   
			   //matamos todas las variables o reiniciamos 
	           
             }
            
            
            
            

    private:



        /***************VARIABLES O CONSTANTES PRIVADAS **********/
        
        /**estructura para la creacion de variables**/
         VarObject varObj[15];
         
		 
		 int  IndexVar = 0 ;
         
         bool ExecVar;
         
         int convertPIN(int pin , int type ){
             
             
             // type = 0 analogo salida
             // type = 1 analogo de entrada
             // type = 2 digital  I/O 
             
             
             if(type == 2 ){
                 
            switch(pin){
                 case 0 :
                    return D0;
                 case 1:
                   return  D1;
                 case 2 : 
                    return D2;
                 case 3: 
                    return D3;
                 case 4 : 
                    return D4;
                 case 5 : 
                    return D5;
                 case 6 : 
                    return D6;
                 case 7 : 
                    return D7;
                }
                 
             }
             else if(type == 0){
                 
                 switch(pin){
                     case 0:
                        return D0;
                     case 1 :
                        return D1;
                     case 2 :
                        return D2;
                     case 3 :
                        return D3;
                      case 4:
                        return A4;
                     case 5 :
                        return A5;
                     case 6 : 
                        return A6;
                     case 7 :
                        return A7;
                }
                 
             }
             else if(type == 1){
                 
                 switch(pin){
                     case 0 :
                        return A0;
                     case 1:
                        return A1;
                     case 2: 
                        return A2;
                     case 3:
                        return A3;
                     case 4:
                        return A4;
                     case 5 :
                        return A5;
                     case 6 : 
                        return A6;
                     case 7 :
                        return A7;
                 }
                 
             }
             
             
             
         }
         
         
         bool InitVariables (bool check ){
             
             ObjSetup object = this->ObDefault_;
             
             //si no existe configuracion entonces retorna un valor falso 
             if(object.variables == NULL 
                        || object.variables == '\0' 
                        || object.variables[0] == '\0'
                        || strcmp(object.variables , "{}")==0 )
                    return false;
             else {
                if(check)
                    if(this->ExecVar )
                        return true ; 
             }
             
             
            
             
             char * variables;
             char * str;
             bool err_flag  = false;
             char * _name   = NULL;
             char * _pin    = NULL;
             char * _type   = NULL;
             char * _format = NULL;
             char * _active = NULL;
			 char * _value  = NULL;

             variables = (char*) malloc(( strlen(object.variables) + 1) * sizeof(char));
             strcpy(variables, object.variables);
             
            
             
             while( str =  strtok_r(variables , " ;" , &variables )){
                 
                 
                     /***
                      * Obtiene la configuracion de las variables en JSON 
                      * se convierte cada valor en un tipo puntero char 
                      * para asi crear el objeto o estructura [ recordar que la estructura es max 15 variables]
                     ***/
                     
                    
                    _name         =  this->ConfJson(str , "n");
                    _pin          = this->ConfJson(str , "p");
                    _type         = this->ConfJson(str , "t");
                    _format       = this->ConfJson(str , "f");
                    _active       = this->ConfJson(str , "a");
					_value		  = this->ConfJson(str, "v");
                    
       
                    bool ok = this->creatVariables(_name, _pin , _type , _format , _active , _value );
                   
                    
                    if(!ok){ err_flag = true; break; }
                    
                  
             }
             
               
             
            /* free(variables);
             free(str);
             free(_name);
             free(_pin);
             free(_type);
             free(_format);
             free(_active);
			 free(_value);*/
             
             
             if(err_flag) return false;
             this->ExecVar = true;
             
             return true;
             
         }
         
        
      
         /**
         * variable que al momento de hacer llamada por el war se agregara la 
         * data en la cola y creara un back-to-back que actuara en un tiempo T 
         * determinado a agregar lo snuevos cambios de variables a la lista 
         * de la eeprom 
        **/
        // deprecada :( 
        // QueueArray <char*> Qvar;
        
        
         //id del dispositivo 
         String deviceId ; 
         
         // nombre del hook
         String  web_service;

         // hook base
         String  web_base ;

         //hook control

         String  web_control ;


         //web services error callback
         String  web_service_error ;

         //hook error callback
         String web_base_error;

         //control service
         String control_service;


         // webserver war
         String web_config                  = "war";


         //tiempo de retardo en milisegundos
         int    tick                        = 2000;


         //creando un loop por medio de timer
         Timer timer;



         //compositor de json
         char  json_compose[700];

         //compositor de consola
         char  console_[700];

         //compositor de errores
         char  error_[700];


         //test mode
         boolean test_mode                  = false;

         boolean default_mode               = false;

         boolean activate_task              = false;


         //variable de iteracion

         int iteration                      = 0;



         /*************FUNCIONES PRIVADAS ************************/

         //webhook trigger
         void GetTrigger (const char *event, const char *data) {};



         //Error handler para errores detectados en el hook
         void GetErrorHandle(const char *event, const char *data) {};


         //Control del dispositivo Handle de accion
         void GetControlHandle(const char *event, const char *data){};


         //Dispositivo de ejecucion de control
         void ExecuteControlDevice(bool status)
         {
             switch(status)
             {
                 case true :
                    break;
                 case false :
                    return;
             }
         }


        //web hook error execute
        void ExecuteErrorHandle(String error) { Particle.publish(web_base_error, error, PRIVATE);  }
        
        
    
         //web hook execute
        void ExecuteHook(String process) {  
            
            Particle.publish(web_base, process, PRIVATE);  
        }


    protected:


         //direccion inicial de la memoria EEPROM
         int addr_      = 1;

         //configuracion por defecto
         ObjSetup ObDefault_;


};



/**
 * Algoritmo que verifica el estado del firmware
 * esto ayuda a verificar si existe problemas o si
 * esta fallando la conexion en un tiempo T dado
***/
int Swater::FirmStatus()
{
    if(WiFi.connecting())
    {
        this->FirmStatus();
        delay(5000);
       // return 0;
    }

    if(WiFi.ready())
    {

        if (!Particle.connected()){
            Particle.connect();
            delay(5000);
            this->FirmStatus();
           //return 0;
        }

    }
    else
        return 0;

    return 1;
}





/**
 * @author Rolando Arriaza 
 * @version 1.1.0
 * @description Configuracion de emergencia 1 --> eemprom por defecto , 0 --> nada
 * 
 *  parche 1.1.0 
 *          -> se crearon nuevas funciones case 2 y 3 
 * 
***/
int Swater::emergencyConfig(String command){
    

    int _execute = atoi(command.c_str());
    
    switch(_execute){
            
            case 0 :  //Vueve a arracar el sistema en caso de emergencia
                this->init(false);
               return 0;
            case 1: //nuevo tick de tiempo , por defecto 
                 ObjSetup o ;
                 o = DefConfig::get_DefConf();
                 this->set_PrimaryConf(o);
                 this->init(false);
                 //creamos un nuevo periodo 
                 this->set_period(o.tick);
                 ObDefault_ = o;
                 //eliminamos las variables en memoria 
                 this->TruncateVariables();
                 this->InitVariables(false);
                break;
           case 2 :  //limpieza de la EEPROM 
                EEPROM.clear();
                return 2;
            case 3 : //reinicia todo el object setup 
                 ObjSetup e ;
                 e = DefConfig::get_DefConf();
                 ObDefault_ = e;
                 return 3;    
            
    }
       
    return 1;
}



/***
 * algoritmo de configuracion al momento que se ejecuta el WAR
 * la ejecucion de este war se viene dado desde la aplicacion web
 * generando asi el servicio respectivo , una vez ejecutado
 * se busca el comando y se ejecuta la funcion asociada
***/
int Swater::set_config(String command){
    
    
    //Existe un maximo de 51 carcateres a capturar , limitante ...


   sprintf(console_, " %s%s%s" , "[C:set_config(command)" , ConvertoChar(command) , "]");


   //ConfJson
   //sobre escritura de la memoria eeprom
   bool ovewrite = false;

   //patron asignado al momento de hacer el split
   char pattern = ';';


   //comandos a analizar
   String cmd   = SplitCommand(command , pattern , 0);
   String exec  = SplitCommand(command , pattern ,  1);


   //objeto ...
   ObjSetup object = ObDefault_;

   //si el comando devuelve nada o vacio entonces generar un error
   if(cmd == "")
   {
       sprintf(error_, " %s%s%s" , "[Error, El comando : " , ConvertoChar(command) , " En set_config(command) ]");
       //ExecuteErrorHandle ( String("[Error, El comando : " +  command +  " no se parametrizo de forma correcta [devuelve nulo] , Error Critico "));
       return -1;
   }
   
   
   
   if(cmd == "TEST_VAR"){
         return this->TestVar(exec.c_str() , false);
   }
   
   if(cmd == "FORCE_VAR" ){
       
       
       bool varInit = false ;
       varInit = this->InitVariables(false);
       
       switch(varInit){
           case true :
                return 1;
           case false :
                return 0;
       }
       
       
       return 0;
       
   }
   
   
   if(cmd == "DELETE_VAR"){
       
       bool success = DeleteVariable(exec.c_str());
         
       switch(success){
           case true :
                return 1;
           case false :
                return 0;
       }
       
       return 0;
       
   }


   if(cmd == "PARTICLE")
   {

      if(exec == "ACTIVATE" && Particle.connected() == false){
             Particle.connect();
             return 1;
      }
      else if(exec == "ACTIVATE" && Particle.connected() ){
           return 1;
      }
      else if(exec == "RESET" ){
           System.reset();
           return 1;
      }
      else if(exec == "SAFE_MODE"){
          System.enterSafeMode();
          return 1;
      }


      return -1;

   }
   else if(cmd == "VAR")
   {
       
       char * _json = (char*) exec.c_str();
       
       //char * _name         = this->ConfJson(_json , "n");
       //char * _pin          = this->ConfJson(_json , "p");
       //char * _type         = this->ConfJson(_json , "t");
      // char * _format       = this->ConfJson(_json , "f");
      // char * _active       = this->ConfJson(_json , "a");
       
      //char * name , char * pin , char* type , char * format , char * active , int index
      // bool ok = this->creatVariables(_name, _pin , _type , _format , _active);
      this->ProgramConf(_json);
      this->InitVariables(false);
        
   }
   else if(cmd == "VERSION"){
       object.version = atoi(exec.c_str());
       ovewrite = true;
       ObDefault_ = object;
   }
   //configuracion de data tipo json 
   else if (cmd == "JCONF"){
       
     
       char * _execute = (char*) exec.c_str();
      
       if(_execute == NULL || _execute == ""){
           return -1;
       }
       
       
        char*   _version            = NULL;
        char*   _wbase              = NULL;
        char *  _wservices          = NULL;
        char *  _activate           = NULL;
        char * _sleep               = NULL;
        char * _tick                = NULL;
        char *  _war                = NULL;
       
       
        _version       =  this->ConfJson( _execute  ,"version");
        _wbase         =  this->ConfJson( _execute , "web_base");
        _wservices     =  this->ConfJson( _execute , "webservices");
        _activate      =  this->ConfJson( _execute , "activate");
        _sleep         =  this->ConfJson( _execute , "sleep");
        _tick          =  this->ConfJson( _execute , "tick");
        _war           =  this->ConfJson( _execute , "war");


       if(_version != NULL){
            object.version = atoi(_version);
       }
       if(_wbase  != NULL && _wbase != ""){
           strcpy(object.web_base, _wbase);
       }
       if(_wservices != NULL && _wservices != ""){
            strcpy(object.webservices, _wservices);
       }
       if(_activate != NULL ){
           object.activate = atoi(_activate);
       }
       if(_sleep != NULL){
          object.sleep = atoi(_sleep);
       }
       if(_tick != NULL && atoi(_tick) > 100 ){
          object.tick = atoi(_tick);
          this->set_period(object.tick);
       }
       if(_war != NULL && _war != ""){
          strcpy(object.war, _war);
       }
       
       
           
     /* free(_version);
      free(_wbase);
      free(_wservices);
      free(_activate);
      free(_sleep);
      free(_tick);
      free(_war);*/

      ObDefault_ = object;

      if(test_mode){
           this->init(false);
      }else ovewrite = true;
 


   }
   else if(cmd == "CONF_RESET"){
        this->emergencyConfig(exec);
   }
   else if(cmd == "STATUS"){
       
       /***
        * STATUS DEL SISTEMA :
        * 
        *       VERIFICA CONFIGURACIONES MEDIANTE COMANDOS     
        *       ESTOS DEVUELVEN UN VALOR ENTERO (CODIGO)
        * 
        *        VAR_VIRTUAL :  --verifica si las variables virtuales estan cargadas 
        *                       --OK            = 100
        *                       --ERROR         = 101 
        *               
        *        VAR_CONF :    -- verifica si el objeto de configuracion OBJSETUP variables no esta vacio
        *                       --OK            = 102
        *                       --ERROR         = 103              
        * 
       ****/
       
       if(exec  == "VAR_VIRTUAL"){
           
            if( this->varObj[0].name == NULL)
                return 101;
            else
                return 100;
            
       }
       else if(exec == "VAR_CONF"){
           if(object.variables == NULL || object.variables == '\0')
              return 103;
            else 
              return 102;
       }
       
       return 999;
       
   }

      //se activo la sobre escritura
      if(ovewrite){
          this->set_PrimaryConf(object);
          this->init(false);
      }

     return 1;
}



int Swater::TestFunction(String command){
    
    int _execute = atoi(command.c_str());
    
 
    
    if(_execute == 1 ){
        
        ObjSetup obj = this->get_PrimaryConf();
        sprintf(console_ , "%s" ,  obj.variables );
    }
    else if(_execute == 2){
        
        bool r = this->InitVariables(false);
        if(r) return 1;
        else return 0;
        
    }
    else if(_execute == 3){
        
        char * a;
        a = NULL;
        //a = (char*) (malloc(100 * sizeof(char)));
        
        for(int i = 0 ; i < 3 ; i++){
            strcat(a , varObj[i].name);
        }
        
        sprintf(error_ , "%s" , a);
        
    }
    else if (_execute == 4){
        if( varObj[0].name == NULL)
        return 99;
        else return 98;
    }
    else if(_execute == 5){
        
        sprintf(error_ , "%s" ,  varObj[0].name);
        sprintf(console_ , "%s" ,  varObj[1].name);
    }
    else if(_execute == 6){
        
        ObjSetup obj = this->get_PrimaryConf();
        sprintf(error_ , "%u" , strlen(obj.variables));
        
    }
    else if(_execute == 7){
        
         ObjSetup obj = this->get_PrimaryConf();
         sprintf(console_ , "%s" , obj.variables[6]);
        
    }
    else if(_execute == 8){
        ObjSetup o = this->get_PrimaryConf();
        sprintf(console_  , "%s" , o.variables[0]);
    }
    
  
 
    return 0;
    
}



/**
 * es un algoritmo de tipo asincrono que se ejecuta independientemente
 * del loop actual , este proceso dura segun la configuracion del usuario
 * ojo !! como es asincrono si existen resultados al llamar en la funcion
 * loop y como sus timers no estan sincronizados puede obtener o establecer
 * datos no deseados.
***/
void Task::task_process(void){}


/**
 * loop es el ciclo principal, todo codigo que se desea ejecutar en un tiempo T
 * debera ir aca en esta funcion. no esta ligada a task. teoricamente loop seguira ejecutandoce
 * de forma independiente.
***/
void Swater::loop(void){

     

      //primera condicion en el loop , verificar el firmware
      if(this->FirmStatus() == 0 ){
           return;
      }
      
      
      String process = String("");
      
      if(this->ExecVar){
          process = this->get_process();
      }
      else{
          process = "{\"error\": \"Variables no iniciadas o no existen\" ,  \"data\": \"null\" ,  \"id\" : \"" + System.deviceID() + "\" }";
      }
      
      
      //String p = String(analogRead(this->convertPIN( varObj[0].pin , 1)) );
      //ExecuteErrorHandle (process);
     
     // EJECUCION DEL WEBSERVICES , SI ESTA COMENTADA ES POR EL SIMPLE HECHO DE HACER PRUEBAS 
      ExecuteHook(process);
     
     //sprintf(console_, "%s" , analogRead(this->convertPIN( varObj[0].pin , 1)) );
     //sprintf(error_, "%u" , varObj[0].pin );
     
}




/***
 * @Author Rolando Arriaza 
 * @Version 1.0 
 * @Todo Esta funcion procesa los datos entregados por el dispositivo o no ? 
 *       el objetivo es que devuelva una cadena tipo json para luego asi enviarla 
 *       al servicio que se tienen , una vez mandada al servicio se procesan los datos 
 *       
 *      OJO : los datos que envia son en crudo , basicamente adimensionales , porque ? 
 *            el backend se encargara de procesar los datos de forma logica si y solo si 
 *            el desarrollador crea las funciones adecuadas 
 * 
 *      FUTURO:
 * 
 *          La variable VI = virtual  no esta programada , ya que en este proyecto beta no se usara 
 *                           pero queda a creatividad del otro desarrollador que le meta mano, 
 *                           la logica de virtual es obtener un dato por medio de algun algoritmo interno del firmware 
 *                           asi que como por ejemplo podria crear el algoritmo siguiente:
 * 
 *          queremos hacer un factor de conversion variable  por medio del voltaje entregado en analogread
 *           0 y 3,3 voltios en valores enteros entre 0 y 4095 
 *          le decimos que el promedio de los valores en read = get_prom() 
 *          
 *                  int get_conversion() {
 *                      
 *                       float max = 3.3;
 *                       int result = 0 ;
 *                       int prom   = this->get_prom();
 *                       
 *                        if(prom == 0 ) { return 0 ;}
 * 
 *                        return (int)  max / prom  ; 
 * 
                    }
                    
            Fragmento del codigo get_process()
            
            
             else if(strcmp(varObj[i].type , "VI") == 0){
                  //Variable Virtual  
                  
                  int vi = this->get_conversion();
                  
                    data = "{";
                    data += "\"value\":\"" ;
                    data += String(vi);
                    data += "\"";
                    data += ",";
                    data += "\"name\":" ;
                    data += "\"";
                    data += varObj[i].name;
                    data += "\"";
                    data += "}";
                    
                    //mamado va :D
                  
              }
              
 * 
 * 
****/
String Swater::get_process()
{
    
        int size                    = sizeof(varObj) / sizeof(*varObj);
        String  deviceId            = this->deviceId;
        
        
        String Jformat = "{"; 
        String data    = String("");
        String val     = String("");
        
        
        
        
        Jformat += "\"error\":\"null\"";
        Jformat += ",";
        Jformat += "\"data\":[";
        
        
        int i = 0;
     
        for(i  ; i < size ; i++){
        
            if(varObj[i].name != NULL && varObj[i].active == true ){
                
                 
                // sprintf(console_ , "%s" , varObj[i].name);
                
                if(
                        strcmp(varObj[i].type , "AI") == 0 
                    ||  strcmp(varObj[i].type , "DI") == 0
                    ){
                        
                    //condicion analogo de entrada      
                
                    data = String("");
                    data = "{";
                    data += "\"value\":\"" ;
                
                    val = "";
                    if( strcmp(varObj[i].type , "AI") == 0 ){
                         val = analogRead(this->convertPIN( varObj[i].pin , 1));
                    }
                    else if(strcmp(varObj[i].type , "DI") == 0){
                        val = digitalRead(this->convertPIN( varObj[i].pin , 2));
                    }
                
                    data += val ;
                    data += "\"";
                    data += ",";
                    data += "\"name\":" ;
                    data += "\"";
                    data += varObj[i].name;
                    data += "\"";
                    data += "}";
                    
                    
                    data += ",";
                   // if(i < (size-1) ){
                  //        data += ",";
                  //    }
                
                    Jformat += data;
              }
              else if(strcmp(varObj[i].type , "AO") == 0 ){
                  //Analogo de salida 
                  int pin_ = this->convertPIN( varObj[i].pin , 0);
                  analogWrite( pin_ , varObj[i].value);
                  
              }
              else if(strcmp(varObj[i].type , "DO") == 0){
                  //Digital de salida 
                   int pinA_ = this->convertPIN( varObj[i].pin , 2);
                   digitalWrite( pinA_ , varObj[i].value);
                  
              }
              else if(strcmp(varObj[i].type , "VI") == 0){
                  //Variable Virtual  
                  // en alguna otra version algun programador podria escribir el codigo 
                  // para la variable virtual , por el momento no interesa
              }
              
              
             
          
              
            }
          
            
            
             
        }
        Jformat  += "],";
        Jformat  += "\"id\":" ;
        Jformat  += "\"" +  String(deviceId) ;
        Jformat  += "\"";
        Jformat  += "}";
        
    
        
        return Jformat;

}



Swater smartwater;

void setup(){
    smartwater.set_period(60*2000);
    //smartwater.start_testMode(true);
    //smartwater.start_defaultMode(true );
    smartwater.init(false) ;
}