#include "loggers.h"

void log_header(t_log *logger, Paquete *paquete, const char *_contexto, ...)
{
    // Esto que sigue es para poder loggear un string con formato
    // Cuando lo pasaba a una funcion loggear_contexto (para usar en cada caso) lo imprimia mal
    // Igual se soluciona escribiendo arriba de log_header, un log_info con lo que quieras y listo
    /*
    if(_contexto != '\0')
        loggear_contexto(logger, _contexto);
    */
    // Por ahora va en cada funcion.
    va_list arguments;
    va_start(arguments, _contexto);
    char *contexto = string_from_vformat(_contexto, arguments);
    log_info(logger, "%s", contexto);

    va_end(arguments);	
    free(contexto);
    // Hasta que es para loggear contexto

    log_debug(logger, "Header.tamPayload: %d", paquete->header.tamPayload);
    log_debug(logger, "Header.tipoMensaje: %s", devolver_tipo(paquete->header.tipoMensaje));
    log_debug(logger, "Header.emisor: %s", Emisores[paquete->header.emisor]);
}

void log_dtb(t_log *logger, DTB *dtb, const char *_contexto, ...)
{
    va_list arguments;
    va_start(arguments, _contexto);
    char *contexto = string_from_vformat(_contexto, arguments);
    log_info(logger, "%s", contexto);

    va_end(arguments);	
    free(contexto);

    log_debug(logger, "dtb->gdtPID: %d", dtb->gdtPID);
    log_debug(logger, "dtb->PC: %d", dtb->PC);
    log_debug(logger, "dtb->flagInicializacion: %d", dtb->flagInicializacion);
    log_debug(logger, "dtb->entrada_salidas: %d", dtb->entrada_salidas);
    
    int i;
    for(i = 0; i < list_size(dtb->archivosAbiertos); i++)
    {
        ArchivoAbierto *archivo = list_get(dtb->archivosAbiertos, i);
        if(i == 0)
            log_archivo(logger, archivo, "ESCRIPTORIO");
        
        log_archivo(logger, archivo, "");
    }
}

void log_archivo(t_log *logger, ArchivoAbierto *archivo, const char *_contexto, ...)
{
    va_list arguments;
    va_start(arguments, _contexto);
    char *contexto = string_from_vformat(_contexto, arguments);
    log_info(logger, "%s", contexto);

    va_end(arguments);	
    free(contexto);

    log_debug(logger, "Archivo->path: %s", archivo->path);
    log_debug(logger, "Archivo->cantidad_lineas: %d", archivo->cantLineas);
    log_debug(logger, "Archivo->segmento: %d", archivo->segmento);
    log_debug(logger, "Archivo->pagina: %d", archivo->pagina);
}

void loggear_contexto(t_log *logger, const char *_contexto, ...)
{
    va_list arguments;
    va_start(arguments, _contexto);
    char *contexto = string_from_vformat(_contexto, arguments);
    log_info(logger, "%s", contexto);

    va_end(arguments);	
    free(contexto);
}

char *devolver_tipo(Tipo tipo_mensaje)
{
    switch(tipo_mensaje)
    {
    // Mensajes generales
    case ESHANDSHAKE: return "ESHANDSHAKE";
    case ESSTRING: return "ESSTRING";
    case ESDATOS: return "ESDATOS";
    case SUCCESS: return "SUCCESS";
    case ERROR: return "ERROR";
    case DTB_FINALIZAR: return "DTB_FINALIZAR";
    // Diego -> MDJ
    case VALIDAR_ARCHIVO: return "VALIDAR_ARCHIVO";
    case CREAR_ARCHIVO: return "CREAR_ARCHIVO";
    case OBTENER_DATOS: return "OBTENER_DATOS";
    case GUARDAR_DATOS: return "GUARDAR_DATOS";
    case BORRAR_ARCHIVO: return "BORRAR_ARCHIVO";
    // Diego -> SAFA
    case DUMMY_SUCCESS: return "DUMMY_SUCCESS";
    case DUMMY_FAIL_CARGA: return "DUMMY_FAIL_CARGA";
    case DUMMY_FAIL_NO_EXISTE: return "DUMMY_FAIL_NO_EXISTE";
    case DTB_SUCCESS: return "DTB_SUCCESS";
    case ARCHIVO_ABIERTO: return "ARCHIVO_ABIERTO";
    case ARCHIVO_CERRADO: return "ARCHIVO_CERRADO";
    // Diego -> FM9
    case ABRIR: return "ABRIR";
    case FLUSH: return "FLUSH";
    // CPU -> FM9
    case NUEVA_PRIMITIVA: return "NUEVA_PRIMITIVA";
    case CLOSE: return "CLOSE";
    case ASIGNAR: return "ASIGNAR";
    // CPU -> SAFA
    case DTB_EJECUTO: return "DTB_EJECUTO";
    case DUMMY_BLOQUEA: return "DUMMY_BLOQUEA";
    case DTB_BLOQUEAR: return "DTB_BLOQUEAR";
    case QUANTUM_FALTANTE: return "QUANTUM_FALTANTE";
    case WAIT: return "WAIT";
    case SIGNAL: return "SIGNAL";
    // SAFA -> CPU
    case ESDTBDUMMY: return "ESDTBDUMMY";
    case ESDTB: return "ESDTB";
    case ESDTBQUANTUM: return "ESDTBQUANTUM";
    case FINALIZAR: return "FINALIZAR";
    case CAMBIO_CONFIG: return "CAMBIO_CONFIG";
    case ROJADIRECTA: return "ROJADIRECTA";
    case SIGASIGA: return "SIGASIGA";
    // FM9 -> CPU
    case LINEA_PEDIDA: return "LINEA_PEDIDA";
    // FM9 -> Diego
    case CARGADO: return "CARGADO";
    // MDJ -> Diego
    case ARCHIVO: return "ARCHIVO";
    // Errores
    // Abrir
    case PATH_INEXISTENTE: return "PATH_INEXISTENTE: 10001";
    case ESPACIO_INSUFICIENTE_ABRIR: return "ESPACIO_INSUFICIENTE_ABRIR: 10002";
    // Asignar
    case ABORTARA: return "ABORTARA: 20001";
    case FALLO_DE_SEGMENTO_ASIGNAR: return "FALLO_DE_SEGMENTO_ASIGNAR: 20002";
    case ESPACIO_INSUFICIENTE_ASIGNAR: return "ESPACIO_INSUFICIENTE_ASIGNAR: 20003";
    // Flush
    case ABORTARF: return "ABORTARF: 30001";
    case FALLO_DE_SEGMENTO_FLUSH: return "FALLO_DE_SEGMENTO_FLUSH: 30002";
    case ESPACIO_INSUFICIENTE_FLUSH: return "ESPACIO_INSUFICIENTE_FLUSH: 30003";
    case ARCHIVO_NO_EXISTE_FLUSH: return "ARCHIVO_NO_EXISTE_FLUSH: 30004";
    // Close
    case ABORTARC: return "ABORTARC: 40001";
    case FALLO_DE_SEGMENTO_CLOSE: return "FALLO_DE_SEGMENTO_CLOSE: 40002";
    // Crear
    case ARCHIVO_YA_EXISTENTE: return "ARCHIVO_YA_EXISTENTE: 50001";
    case ESPACIO_INSUFICIENTE_CREAR: return "ESPACIO_INSUFICIENTE_CREAR: 50002";
    case ARCHIVO_NO_EXISTE: return "ARCHIVO_NO_EXISTE: 60001";

    default: return "No reconoci el tipoMensaje";
    }
}