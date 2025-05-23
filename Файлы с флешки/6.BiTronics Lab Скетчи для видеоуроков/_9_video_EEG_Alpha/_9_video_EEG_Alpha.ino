/*
EEG_Alpha
Оцифровывает аналоговый сигнал электроэнцефалограммы со входа А0 платы Arduino, осуществляется разложение сигнала в спектр (прямое преобразование Фурье). 
При превышении альфа-ритмом (соответствует частотным компонентам 8-12 Гц сигнала ЭЭГ) порогового значения - зажигается светодиод
или активируется вращение мотора (в зависимости от того, что подключено к управляемому выводу).
Считанный сигнал отображается в программе визуализации от BiTronics, скачать ее можно тут: http://www.bitronicslab.com/guide/ 
Справочник по командам языка Arduino: http://arduino.ru/Reference 
*/
#include <fft.h>                       // подключаем библиотеку для разложения сигнала в спектр (по алгоритму "Быстрого преобразования Фурье"),  
                                       // скачиваем ее отсюда и устанавливаем: https://www.dropbox.com/s/vw46osetrwgft43/FFT.zip
#define num 256                        // зададим число оцифрованных значений сигнала ЭЭГ, которое будет отправлено в функцию для разложения в спектр
#define FAN_PIN 7                      // определим номер вывода Arduino, к которому подключается мотор или светодиод
                                       // в случае красного пропеллера к данному выводу должен быть подключен вывод IN-A пропеллера
#define ERROR_LED_PIN 3                // определим номер вывода Arduino, к которому подключается светодиод, горение которого обозначает высокий уровень помех
float threshold = 11;                  // порог срабатывания светодиода или мотора красного пропеллера (может требовать подстройки под конкретного человека)
int8_t im[num], data[num];             // массивы для накопления данных
int  i = 0;                            // переменная-счетчик 
int val = 0;
byte sped = 0;
bool fail = false;                     // логическая (булева) переменная, значение которой равно ИСТИНА при высоком уровне помех
float specter = 0;                     // переменная для хранения текущего значения спектра
float specter_old = 0;                 // переменная для хранения предыдущего значения спектра

void setup() {
  Serial.begin(115200);                // инициализируем Serial-порт на скорости 115200 Кбит/c. 
                                       // такую же скорость надо установить в программе для визуализации
  pinMode(FAN_PIN, OUTPUT);            // конфигурируем вывод FAN_PIN на Arduino как выход, см. описание  pinMode() http://arduino.ru/Reference/PinMode
  pinMode(ERROR_LED_PIN, OUTPUT);      // конфигурируем вывод ERROR_LED_PIN на Arduino как выход
}

void loop() {
  int8_t sum = 0;                      // переменная для хранения результата суммирования значений сигнала ЭЭГ
  for (i = 0; i < num; i++) {          // создаем массив 
  val = analogRead(A0);                // записываем в переменную val оцифрованное значение сигнала с ножки А0 на Arduino.
                                       // val может принимать значение в диапазоне от 0 до 1023, см. http://arduino.ru/Reference/AnalogRead                              
  data[i] = val/8;                     // делим на 8 для того, чтобы диапазон значений val соответствовал размеру элемента массива data[]  
  Serial.write("A0");                  // записываем в Serial-порт имя поля в программе для визуализации, куда надо выводить сигнал
                                       // всего в этой программе 4 поля, которые имеют имена A0, A1, A2, A3 (сверху вниз, по порядку их расположения в окне программы)
  Serial.write(map(val, 0, 1023, 0, 255)); // отправляем результат оцифровки в Serial-порт   
    if (data[i] < 2 || data[i] > 120) {// если считанное значение сигнала меньше 2 или больше 120 - считаем, что очень высокий уровень помех в сигнале (не забываем, что значения с А0 разделены на 8)  
      fail = true;                     // в этом случае записываем в переменную fail истинное значение
    }
    delay(2);                          // ждем 2 миллисекунды (1000 миллисекунд = 1 секунда); по-сути, эта величина задает период считывания сигнала со входа А0
    im[i] = 0;                         // обнуляем элементы массива im[]                                      
    sum = sum + data[i];               // прибавляем считанное значение сигнала data[i] к уже накопленной сумме ранее просуммированных значений сигнала
  }
 
 if (!fail) {                          // условие, обозначающее, что значения сигнала не зашкаливают
   digitalWrite(ERROR_LED_PIN, LOW);   // если уровень шума низкий - выключаем светодиод, подключенный к выводу ERROR_LED
   for (i=0; i < num; i++) {                                     
     data[i] = data[i] - sum/num;      // удаляем из записанных в массив data[i] значений сигнала постоянную составляющую (требования работы библиотеки для разложения в спектр fft.h,                    
                                       // для этого из каждого значения массива data[i] вычитаем среднее значение сигнала по всем точкам, накопленным в этом массиве
  }
 // теперь запускаем функцию fix_fft() из библиотеки fft.h для разложения сигнала в спектр
   fix_fft(data, im, 8, 0);            // эта функция осуществляет разложение сигнала в спектр: data - массив с исходными данными, в данном массиве также будет возвращена
                                       // действительная часть результата преобразования Фурье. В массиве im  возвращается мнимая часть результата преобразования Фурье. 
                                       // 8 - обозначает степень числа 2 - задает число спектральных компонент для разложения сигнала (2^8=256 значений)
                                       // 0 - означает, что надо выполнять прямое преобразование Фурье (существует еще обратное преобразование Фурье - задается значением 1)
                                       // подробнее о библиотеке: https://github.com/TJC/arduino/blob/master/sketchbook/libraries/fix_fft/fix_fft.h
                                       // подробнее о преобразовании Фурье: https://habrahabr.ru/post/196374/ 
    bool flag = false;                 // логическая (булева) переменная. ИСТИНА (true) - если значение альфа-ритма выше порогового значения threshold
    specter_old = specter; 
    specter = 0;     
   for (i = 4; i < 8; i++){            // в массивах со спектральными компонентами ЭЭГ сигнала data[] и im[]
                                       // перебираем частотные компоненты с 4 до 8 - эти спектральные компоненты соответствуют альфа-ритму
     specter +=  sqrt(data[i]*data[i] + im[i]*im[i]); // и суммируем эти спектральные компоненты, которые вычисляем как модуль комплексного числа: https://ru.wikipedia.org/wiki/Комплексное_число#Модуль
   }
   specter = 0.3 * specter + 0.7 * specter_old; // сглаживание сигнала, величину сглаживания выбираем коэффициентами перед переменными specter и specter_old, необходимо чтобы их сумма всегда равнялась единице
// Serial.println(specter);            // если раскомментировать эту строку и закоментировать выше строки с Serial.write(), то можно в терминале Serial-порта смотреть на значения альфа-ритма
      if(specter > threshold){         // значение альфа-ритма выше порогового значения threshold
        flag = true;                   // записываем в логическую переменную flag значение ИСТИНА (true)
      }
// нижеследующие условия позволяют плавно снижать обороты мотора, если амплитуда альфа-ритма опускается ниже порогового значения threshold
    if (!flag) {                      // если альфа-ритм не превышал значение порогового значения threshold
      if (sped != 0) {                // и мотор вращается (см. определение переменной sped ниже)
        sped = sped - 25;             // снижаем величину скорости на 25 (можно варьировать это значение, чтобы увеличить/уменьшить скорость вращения)
      }
    }
    if(flag) {                        // если альфа-ритм превысил значение порогового значения threshold
      sped = 200;                     // установим переменную sped, задающую скорость вращения мотора или яркость зажигания светодиода, в 200
      flag = false;                   // и сбросим флаг-признак превышения альфа-ритмом порогового значения threshold
    }
    analogWrite(FAN_PIN, sped);        // устанавливаем скорость вращения мотора (яркость свечения светодиода), по analogWrite() см. здесь: http://arduino.ru/Reference/AnalogWrite
    
 }
 else {                               // данный else выполняется, если уровень шума слишком высокий: fail = ИСТИНА
   digitalWrite(ERROR_LED_PIN, HIGH); // включаем светодиод, оповещающий о высоком уровне шума
   analogWrite(FAN_PIN, 0);           // останавливаем вращение мотора или тушим светодиод
 }
 fail = false;                        // сбрасываем значение fail в состояние, соответствующее отсутствию шумов и отправляемся на следующий виток цикла loop()
}

