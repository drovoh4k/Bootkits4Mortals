# 🦑 Desarrollo de Bootkits para Mortales

![Bootkit Image](Images/bootkit_cartografo.png)

> Si yo pude, tú también (probablemente)


## 📜 Sobre el proyecto

Este proyecto nace con el objetivo de desmitificar la complejidad inherente al desarrollo de bootkits. La información disponible sobre este tema suele estar fragmentada y ser excesivamente técnica, lo que crea una barrera de entrada para principiantes.

Este repositorio proporciona una guía introductoria y práctica que abarca desde los conceptos teóricos básicos hasta la creación de un bootkit funcional, sirviendo como un punto de partida claro y accesible para cualquier persona interesada en este mundo.


## 🚀 Cómo Empezar

Para poder compilar y ejecutar el código de este repositorio, es fundamental configurar el entorno de trabajo. En los anexos del proyecto encontrarás guías detalladas para cada uno de los pasos principales:

1. **Preparar las Máquinas Virtuales:**
    - El anexo A detalla el proceso de instalación y configuración de una máquina virtual Windows de forma desatendida.

2. **Configurar el Entorno de Desarrollo de Bootkits:**
    - El anexo B explica paso a paso cómo instalar y configurar el entorno EDK2 junto con Visual Studio 2019 y otras dependencias (Git, Python, nasm) para compilar aplicaciones UEFI.


## 📂 Estructura del Proyecto

El código está organizado en carpetas que representan la progresión del trabajo, desde una aplicación básica hasta un bootkit funcional con capacidad de interactuar con el sistema de ficheros NTFS.

* **`01-HelloWorldUEFI`**
    - Una aplicación UEFI básica "Hello World" diseñada para familiarizarse con el entorno de desarrollo EDK2 y su jerarquía de compilación.
* **`02-LoadDriverNTFS`**
    - Una aplicación UEFI que carga un driver para poder leer y escribir en particiones con formato NTFS, permitiendo así la interacción con el sistema de ficheros de Windows desde el entorno UEFI.
* **`03-ExfiltrationHostsNTFS`**
    - Un bootkit que aprovecha el driver NTFS para leer y escribir ficheros clave.
* **`Scripts`**
    - Scripts de automatización para optimizar ciertos procesos.


## 🛣️ Siguientes Pasos

Este proyecto sienta las bases, pero es solo la punta del iceberg. La evolución natural, como se discute en el quinto capítulo, consiste en desarrollar un loader capaz de inyectar un rootkit interceptando la cadena de arranque de Windows.

El anexo C es una guía para preparar un entorno de desarrollo de rootkits usando Visual Studio 2022 y WDK.

Además, para quienes deseen profundizar, se recomienda encarecidamente estudiar los siguientes proyectos:

- **[Abyss](https://github.com/TheMalwareGuardian/Abyss):** Un framework de investigación centrado en técnicas avanzadas de bootkits.

- **[Benthic](https://github.com/TheMalwareGuardian/Benthic):** Un framework de investigación enfocado en técnicas avanzadas de rootkits.


## 📨 Contacto

Este proyecto representa la guía que me habría gustado tener y no encontré cuando empecé en este mundo. No te ahogarás en información; al contrario, he filtrado y ordenado solo lo esencial para que construyas tu primer bootkit. El objetivo es simple pero poderoso: llevarte de 0 a 1. Y como muchos sabemos, ese primer paso es a menudo el más difícil de todos.

Soy [![LinkedinLogo](https://i.sstatic.net/gVE0j.png) Llorenç](https://www.linkedin.com/in/llorenc-garcia-merino/), quien ha puesto toda su ilusión en este proyecto. Si tienes cualquier tipo de duda, comentario o propuesta de mejora, simplemente contáctame. Estaré más que dispuesto a dedicarte el tiempo necesario para profundizar en cualquier aspecto o ~crear el próximo gran bootkit~ iniciar un proyecto ético juntos.

