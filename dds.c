/* DDS MINI NO DISP */

#define F_CPU 8000000L
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>   // potrzebne do funkcji pow(a,x)


////////////////////////////////
//   WYSYŁANIE DANYCH DO DDS  //
////////////////////////////////
void sendDataToDDS(long delta_reg)
{
    // PORTB.5 DATA, PORTB.4 CLK, PORTB.3 FQ-UP
    #define DDS_DATA_0      PORTB &= 0b011111;
    #define DDS_DATA_1      PORTB |= 0b100000;
    #define DDS_CLK_ON      PORTB |= 0b010000;
    #define DDS_CLK_OFF     PORTB &= 0b101111;
    #define DDS_FQ_UD_ON    PORTB |= 0b001000;
    #define DDS_FQ_UD_OFF   PORTB &= 0b110111;

    long bit_id             = 0;  // kolejny bit z z 40-sto bitowego słowa wgrywany do DDS

    bit_id = 0;

    while (bit_id < 40) {

        if ( bit_id < 32 )
        {
            if ((delta_reg & 0b1))  DDS_DATA_1;
            if (!(delta_reg & 0b1)) DDS_DATA_0;
            delta_reg >>= 1;
        }else{
            DDS_DATA_0;
        }

        DDS_CLK_ON;
        DDS_CLK_OFF;
        bit_id++;
    }
    DDS_FQ_UD_ON;
    DDS_FQ_UD_OFF;
}// end send_data()


////////////////////////////////
//      OBLICZANIE DELTY      //
////////////////////////////////
long countDelta(long frq)
{

    int  dig_id          = 8;  // numer cyfry z częstotliwości
    int  dig_val;        // tutaj wpadają kolejne cyfry z częstotliwości
    int  dds_clk         = 75; // CZĘSTOTLIWOŚĆ GENERATORA KWARCOWEGO W DDS [MHz]             <---------------------------------------------- tutaj poprawić
    long delta;          // liczba wprowadzana do dds
    long delta_mod       = pow(2,32) / dds_clk * 10; // delta dla 10MHz  // delta dla 10MHz

    int frq_digits[7];

    dig_id               = 7; // idziemy od jedynek herców w strone megaherców

    while (dig_id >= 0) {
        dig_val = frq % 10;
        frq /= 10;
        frq_digits[ dig_id ] = dig_val;

        dig_id--;
    }

    delta         = 0; // zerujemy po poprzednich obliczeniach
    dig_id         = 0; // idziemy od dziesiątek megaherców w stronę herców

    while(dig_id < 8){
        delta += delta_mod * frq_digits[ dig_id ];
        delta_mod /= 10;
        dig_id++;
    }
    return delta;
}




int main(void)
{
    #define STEP_LED_ALL_ON           PORTD |= 0b00000111;
    #define STEP_LED_ALL_OFF          PORTD &= 0b11111000;

    #define STEP_LED_1_ON             PORTD |= 0b00000001;
    #define STEP_LED_2_ON             PORTD |= 0b00000010;
    #define STEP_LED_3_ON             PORTD |= 0b00000100;

    long frq                 = 3722000; // [Hz] CZĘSTOTLIWOŚĆ STARTOWA SYNTEZY (czyli odbiorcza, bez uwzględnienia pośredniej!!!)  <---------------------------------------------- tutaj poprawić

    long dly_1                = 0;     // licznik do krótkiego przytrzymania

    int  lock_1              = 0;     // blokada do klawiszy
    int  lock_2              = 0;     // blokada do klawiszy

    int  step                  = 50; // krok przestrajania
    int  dds_st             = 1;  // zmienna pomocnicza ustawiana na 1 wtedy, gdy trzeba przeliczyć deltę po zmianie częstotliwości klawiaturą

    //przeładowanie portów od dds-a bo sie jakies cuda dzieją zaraz po włączeniu
    DDRB  = 0b000000; PORTB = 0b111111;        _delay_ms(200);        // sterowanie DDS-em
    DDRB  = 0b111111; PORTB = 0b000000;        _delay_ms(200);

    DDRC  = 0b000000; PORTC = 0b111111;        // guziki
    DDRD  = 0b11111111; PORTD = 0b00000000;    // diody

    //zamigagnie diodek
    int a = 1;
    for (a = 1;a<=2; a++) {
        STEP_LED_ALL_ON; _delay_ms(150);
        STEP_LED_ALL_OFF; _delay_ms(150);
    }

    STEP_LED_ALL_OFF;
    STEP_LED_2_ON; // 50Hz

    /* Początek nieskończonej pętli */
    while(1)
    {

        if(frq < 3500000)     frq = 3500000;// TUTAJ SIĘ USTAWIA ZAKRES PRZESTRAJANIA   <---------------------------------------------- tutaj poprawić
        if(frq > 3800000)     frq = 3800000;// TUTAJ SIĘ USTAWIA ZAKRES PRZESTRAJANIA   <---------------------------------------------- tutaj poprawić


        ////////////////////////////////
        // PROCEDURA WYLICZANIA DELTY //
        ////////////////////////////////
        if (dds_st != 0)
        { // jeśli zmieniono częstotliwość
            sendDataToDDS( countDelta( frq ) );// ustawiamy Fout
            dds_st = 0; //zablokuj liczenie do czasu kolejnej zmiany częstotliwości
        }//koniec procedury delty


        ////////////////////////////////
        //       PRZYCISK step      //
        ////////////////////////////////
        if( !(PINC & 0b100000) ) // guzik step PINC.5
        {
            if(lock_1 == 0)
            {
                if(lock_2 == 1000)
                {
                    lock_1 = 1;

                    STEP_LED_ALL_OFF; //diody

                    if(step == 10)
                    {
                        step = 50;
                        STEP_LED_2_ON;
                    }
                    else if(step == 50)
                    {
                        step = 1000;
                        STEP_LED_3_ON;
                    }
                    else
                    {
                        step = 10;
                        STEP_LED_1_ON;
                    }
                }
                lock_2++;
            }
        }


        ////////////////////////////////
        //        PRZYCISK  UP        //
        ////////////////////////////////
        //else if( !(PINC & 0b000001) )  //tune up
        else if( !(PINC & 0b010000) )  //tune up PINC.4
        {
            if(lock_1 == 0) // to działa od razu po kliknięciu w guzik
            {
                if(lock_2 == 1000)
                {
                    frq         += step;
                    dds_st     = 1;
                    lock_1     = 1;
                }
                lock_2++;
            }
            if(dly_1 == 45000) // to działa po krótkim opóżnieniu
            {
                frq         += step;
                dly_1   = 44000;
                dds_st    = 1;
            }
            dly_1++;
        }


        ////////////////////////////////
        //       PRZYCISK  DOWN       //
        ////////////////////////////////
        //else if( !(PINC & 0b000010) ) //tune down
        else if( !(PINC & 0b001000) ) //tune down PINC.3
        {
            if(lock_1 == 0) // to działa od razu po kliknięciu w guzik
            {
                if(lock_2 == 1000)
                {
                    frq         -= step;
                    dds_st     = 1;
                    lock_1     = 1;
                }
                lock_2++;
            }

            if(dly_1 == 45000) // to działa po krótkim opóżnieniu
            {
                frq         -= step;
                dly_1      = 44000;
                dds_st    = 1;
            }
            dly_1++;
        }
        ////////////////////////////////
        //  PO ZWOLNIENIU PRZYCISKÓW  //
        ////////////////////////////////
        else
        {
            dly_1        = 0;
            lock_1    = 0;
            lock_2    = 0;
        }
    }// end main loop



}// end main()

