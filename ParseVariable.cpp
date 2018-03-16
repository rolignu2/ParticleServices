#include <aJSON.h>

class SmartVariables {

  public :

      /**
         @author Rolando Arriaza
         @version 1.0.0
         @description devuelve un valor de la configuracion json
         @param char* jsonString cadena que contiene el json {}
         @param char* type , tipo de dato a buscar en el json { "version " : 1 }
                      donde version vamos a buscar
         @return char* el valor de la llave encontrada version:1 ->devuelve 1
      */
      char * ConfJson(char * jsonString , char * type){

          aJsonObject* root = aJson.parse(jsonString);

         if(root == NULL)
              return NULL;

          aJsonObject* query = aJson.getObjectItem(root, type);

          return query->valuestring;
      }


      char** parseVars(char* jsonString , char ** patterns ){

          this->last_json = jsonString;

          return NULL;
      }
      
      

      char * get_lastJson(){ return this->last_json; }


      char * parseJson(char *jsonString){
          char* value;

          aJsonObject* root = aJson.parse(jsonString);

          if (root != NULL) {
              //Serial.println("Parsed successfully 1 " );
              aJsonObject* query = aJson.getObjectItem(root, "query");

              if (query != NULL) {
                  //Serial.println("Parsed successfully 2 " );
                  aJsonObject* results = aJson.getObjectItem(query, "results");

                  if (results != NULL) {
                      //Serial.println("Parsed successfully 3 " );
                      aJsonObject* item = aJson.getObjectItem(results, "item");

                      if (item != NULL) {
                          //Serial.println("Parsed successfully 4 " );
                          aJsonObject* title = aJson.getObjectItem(item, "title");

                          if (title != NULL) {
                              //Serial.println("Parsed successfully 5 " );
                              value = title->valuestring;
                          }
                      }
                  }
              }
          }

          if (value) {
              return value;
          } else {
              return NULL;
          }
      }

    private :

         char * last_json;

};