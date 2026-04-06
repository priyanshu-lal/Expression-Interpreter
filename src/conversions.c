#include "registry.h"

// UNIT: Length
static double AngstromToM(double n)  { return n * 1e-10; }
static double NanometerToM(double n) { return n * 1e-9; }
static double MicronToM(double n)    { return n * 1e-6; }
static double KmToM(double n)        { return n * 1000.0; }
static double InchToM(double n)      { return n * 0.0254; }
static double CmToM(double n)        { return n / 100.0; }
static double FootToM(double n)	     { return n * 0.3048; }
static double YardToM(double n)      { return n * 0.9144; }
static double MileToM(double n)      { return n * 1609.344; }
// -----------
static double MeterToAngstrom(double n)  { return n * 1e+10; }
static double MeterToNanometer(double n) { return n * 1e+9; }
static double MeterToMicron(double n)    { return n * 1e+6; }
static double MeterToKm(double n)        { return n / 1000.0; }
static double MeterToInch(double n)      { return n / 0.0254; }
static double MeterToCm(double n)        { return n * 100.0; }
static double MeterToFoot(double n)      { return n / 0.3048; }
static double MeterToYard(double n)      { return n / 0.9144; }
static double MeterToMile(double n)	     { return n / 1609.344; }
// End: Length


// UNIT: Volume
static double MLToCM(double n)     { return n * 10e-6; }
static double CCToCM(double n)     { return n * 10e-6; }
static double LToCM(double n)      { return n * 0.001; }
static double OunceToCM(double n)  { return n * 2.957 * 10e-5; }
static double CupToCM(double n)    { return n * 2.365 * 10e-4; }
static double PintToCM(double n)   { return n * 4.731 * 10e-4; }
static double GallonToCM(double n) { return n * 3.785 * 10-3; }
static double CInchToCM(double n)  { return n * 1.638 * 10-5; }
static double CFootToCM(double n)  { return n * 0.02831; }
static double CYardToCM(double n)  { return n * 0.7645; }
// ------------
static double CmToML(double n)     { return n * 1e+6; }
static double CmToCC(double n)     { return n * 1e+6; }
static double CmToL(double n)      { return n * 1000.0; }
static double CmToOunce(double n)  { return n * 33814; }
static double CmToCup(double n)    { return n * 4226.75; }
static double CmToPint(double n)   { return n * 2113.38; }
static double CmToGallon(double n) { return n * 264.17; }
static double CmToCInch(double n)  { return n * 61023.7; }
static double CmToCFoot(double n)  { return n * 35.3147; }
static double CmToCYard(double n) { return n * 1.30795; }
// End: Volume


// UNIT: Weight & Mass
static double MgToKg(double n)     { return n * 1e-6; }
static double CgToKg(double n)     { return n * 1e-5; }
static double DgToKg(double n)     { return n * 1e-4; }
static double GToKg(double n)      { return n * 1e-3; }
static double MTonneToKg(double n) { return n * 1e+2; }
static double PoundToKg(double n)  { return n * 0.45359; }
static double OzToKg(double n)     { return n * 0.02835; }
// -------------------
static double KgToMg(double n)     { return n * 1e-3; }
static double KgToCg(double n)     { return n * 1e+5;}
static double KgToDg(double n)     { return n * 1e+4; }
static double KgToG(double n)      { return n * 1e+3; }
static double KgToMTonne(double n) { return n * 1e-3; }
static double KgToPound(double n)  { return n * 2.20462; }
static double KgToOz(double n)     { return n * 35.27396; }
// End: Weight & mass


// UNIT: Temperature
static double CelsiusToKelvin(double n)    { return 273.15 + n; }
static double FahrenheitToKelvin(double n) { return (n - 32.0) * 5.0 / 9.0 + 273.15; }
// -----------------
static double KelvinToCelsius(double n)    { return n - 273.15; }
static double KelvinToFahrenheit(double n) { return (n - 273.15) * 9.0 / 5.0 + 32.0; }
// End: Teemperature


// UNIT: Energy
static double EVToJoule(double n)     { return n * 1.602 * 1e-19; }
static double CalToJoule(double n)    { return n * 4.184; }
static double KcalToJoule(double n)   { return n * 4184.0; }
static double KjouleToJoule(double n) { return n * 1e+3; }
static double KwhToJoule(double n)    { return n * 3600000.0; }
// --------------
static double JouleToEv(double n)     { return n * 6.242 * 10e+18; }
static double JouleToCal(double n)    { return n / 4.184; }
static double JouleToKcal(double n)   { return n / 4.184; }
static double JouleToKjoule(double n) { return n * 0.001; }
static double JouleToKwh(double n)    { return n / (3.6 * 1e+6); }
// End: Energy


// UNIT: Area
static double SqMmToSqM(double n)    { return n * 1e-6; }
static double SqCmToSqM(double n)    { return n * 1e-4; }
static double HectareToSqM(double n) { return n * 1e+4; }
static double SqKmToSqM(double n)    { return n * 1e+6; }
static double SqInchToSqM(double n)  { return n * 0.00064516; }
static double SqFeetToSqM(double n)  { return n * 0.0092903; }
static double AcreToSqM(double n)    { return n * 4046.86; }
static double SqMileToSqM(double n)  { return n * 2589988.0; }
// -----------
static double SqMToSqMm(double n)    { return n * 1e+6; }
static double SqMToSqCm(double n)    { return n * 1e+4; }
static double SqMToHectare(double n) { return n * 1e-4; }
static double SqMToSqKm(double n)    { return n * 1e-6; }
static double SqMToSqInch(double n)  { return n * 1550.003; }
static double SqMToSqFeet(double n)  { return n * 10.7639; }
static double SqMToAcre(double n)    { return n * 1.19599; }
static double SqMToSqMile(double n)  { return n * 3.86e-7; }
// End: Area


// UNIT: Speed
static double KmphToMps(double n) { return n * 0.2778; }
static double MphToMps(double n)  { return n * 0.44704; }
static double KnToMps(double n)   { return n * 0.5144; }
static double CpsToMps(double n)  { return n * 0.01; }
static double FpsToMps(double n)  { return n * 0.00008467; }
// -------------
static double MpsToKmph(double n) { return n * 3.6; }
static double MpsToMph(double n)  { return n * 2.23694; }
static double MpsToKn(double n)   { return n * 1.94384; }
static double MpsToCps(double n)  { return n * 100.0; }
static double MpsToFps(double n)  { return n * 11811.02; }
// End: Speed


// UNIT: Time
static double MicrosecToSec(double n) { return n * 1e-6; }
static double MillisecToSec(double n) { return n * 1e-3; }
static double MinuteToSec(double n)   { return n * 60; }
static double HourToSec(double n)     { return n * 3600.0; }
static double DayToSec(double n)      { return n * 86400.0; }
static double WeekToSec(double n)     { return n * 604800.0; }
static double YearToSec(double n)     { return n * 31557600.0; }
// ------------
static double SecToMicrosec(double n) { return n * 1e+6; }
static double SecToMillisec(double n) { return n * 1e+3; }
static double SecToMinute(double n)   { return n / 60.0; }
static double SecToHour(double n)     { return n / 3600.0; }
static double SecToDay(double n)      { return n / 86400.0; }
static double SecToWeek(double n)     { return n / 604800.0; }
static double SecToYear(double n)     { return n / 31557600.0; }
// End: Time


// UNIT: Power
static double KwToWatt(double n)        { return n * 1e+4; }
static double HpToWatt(double n)        { return n * 745.7; }
static double PsToWatt(double n)        { return n * 735.5; }
static double BTUPerMinToWatt(double n) { return n * 17.58; }
static double CalPerSecToWatt(double n) { return n * 4.184; }
// ------------
static double WattToKw(double n)        { return n / 1000.0; }
static double WattToHp(double n)        { return n / 745.7; }
static double WattToPs(double n)        { return n / 735.5; }
static double WattToBTUPerMin(double n) { return n / 17.55; }
static double WattToCalPerSec(double n) { return n / 4.184; }
// End: Power


// UNIT: Pressure
static double AtmToPa(double n)  { return n * 101325.0; }
static double BarToPa(double n)  { return n * 1e+5; }
static double KPaToPa(double n)  { return n * 1e+3; }
static double MmHgToPa(double n) { return n * 133.322; }
static double PsiToPa(double n)  { return n * 6894.76; }
// --------------
static double PaToAtm(double n)  { return n / 101.325; }
static double PaToBar(double n)  { return n / 1e+5; }
static double PaToKPa(double n)  { return n / 1e+3; }
static double PaToMmHg(double n) { return n / 133322.0; }
static double PaToPsi(double n)  { return n / 6894.76; }
// End: Pressure


// UNIT: Angle
#define PI_V 3.141592653589793
static double DegToRad(double n)     { return n * PI_V / 180.0; }
static double GradToRad(double n)    { return n * PI_V / 200.0; }
static double ArcminToRad(double n) { return n * PI_V / 10800.0; }
static double ArcsecToRad(double n)  { return n * PI_V / 648000.0; }
static double RevToRad(double n)     { return n * 2 * PI_V; }
// -----------
static double RadToDeg(double n)     { return n * 180.0 / PI_V; }
static double RadToGrad(double n)    { return n * 200.0 / PI_V; }
static double RadToArcmin(double n) { return n * 10800.0 / PI_V; }
static double RadToArcsec(double n)  { return n * 648000.0 / PI_V; }
static double RadToRev(double n)     { return n * 1 / 2 * PI_V; }
// End: Angle

hashmap* g_unitsTable;

void setUnits() {
	// UNIT: Length
	hashmap_set(g_unitsTable, &(Unit) {
		.key ="m",
		.fullName = "Meters",
		.type = UNIT_LENGTH,
		.isPrimary = true,
		.toPrimary = NULL,
		.fromPrimary = NULL
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "A",
		.fullName = "Angstrom",
		.type = UNIT_LENGTH,
		.isPrimary = false,
		.toPrimary = AngstromToM,
		.fromPrimary = MeterToAngstrom,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "nm",
		.fullName = "Nanometers",
		.type = UNIT_LENGTH,
		.isPrimary = false,
		.toPrimary = NanometerToM,
		.fromPrimary = MeterToNanometer,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "micron",
		.fullName = "Micron",
		.type = UNIT_LENGTH,
		.isPrimary = false,
		.toPrimary = MicronToM,
		.fromPrimary = MeterToMicron,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key ="km",
		.fullName = "Kilometers",
		.type = UNIT_LENGTH,
		.isPrimary = false,
		.toPrimary = KmToM,
		.fromPrimary = MeterToKm
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "inch",
		.fullName = "Inch",
		.type = UNIT_LENGTH,
		.isPrimary = false,
		.toPrimary = InchToM,
		.fromPrimary = MeterToInch,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cm",
		.fullName = "Centimeters",
		.type = UNIT_LENGTH,
		.isPrimary = false,
		.toPrimary = CmToM,
		.fromPrimary = MeterToCm,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "foot",
		.fullName = "Foots",
		.type = UNIT_LENGTH,
		.isPrimary = false,
		.toPrimary = FootToM,
		.fromPrimary = MeterToFoot,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "yard",
		.fullName = "Yards",
		.type = UNIT_LENGTH,
		.isPrimary = false,
		.toPrimary = YardToM,
		.fromPrimary = MeterToYard,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "mile",
		.fullName = "Miles",
		.type = UNIT_LENGTH,
		.isPrimary = false,
		.toPrimary = MileToM,
		.fromPrimary = MeterToMile,
	});
	// End: Length

	// UNIT: Volume
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cubic_m",
		.fullName = "Cubic meters",
		.type = UNIT_VOLUME,
		.isPrimary = true,
		.toPrimary = NULL,
		.fromPrimary = NULL,
	});
	
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "ml",
		.fullName = "Milliliters",
		.type = UNIT_VOLUME,
		.isPrimary = false,
		.toPrimary = MLToCM,
		.fromPrimary = CmToML,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cc",
		.fullName = "Cubic CM",
		.type = UNIT_VOLUME,
		.isPrimary = false,
		.toPrimary = CCToCM,
		.fromPrimary = CmToCC,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "l",
		.fullName = "Liters",
		.type = UNIT_VOLUME,
		.isPrimary = false,
		.toPrimary = LToCM,
		.fromPrimary = CmToL,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "ounce",
		.fullName = "Ounces",
		.type = UNIT_VOLUME,
		.isPrimary = false,
		.toPrimary = OunceToCM,
		.fromPrimary = CmToOunce,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cup",
		.fullName = "Cups (US)",
		.type = UNIT_VOLUME,
		.isPrimary = false,
		.toPrimary = CupToCM,
		.fromPrimary = CmToCup,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "pint",
		.fullName = "Pints (US)",
		.type = UNIT_VOLUME,
		.isPrimary = false,
		.toPrimary = PintToCM,
		.fromPrimary = CmToPint,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "gallon",
		.fullName = "Gallons (US)",
		.type = UNIT_VOLUME,
		.isPrimary = false,
		.toPrimary = GallonToCM,
		.fromPrimary = CmToGallon,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cubic_inch",
		.fullName = "Cubic inches",
		.type = UNIT_VOLUME,
		.isPrimary = false,
		.toPrimary = CInchToCM,
		.fromPrimary = CmToCInch,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cubic_foot",
		.fullName = "Cubic feet",
		.type = UNIT_VOLUME,
		.isPrimary = false,
		.toPrimary = CFootToCM,
		.fromPrimary = CmToCFoot,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cubic_yard",
		.fullName = "Cubic Yards",
		.type = UNIT_VOLUME,
		.isPrimary = false,
		.toPrimary = CYardToCM,
		.fromPrimary = CmToCYard,
	});
	// End: Volume

	// UNIT: Weight & mass
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "kg",
		.fullName = "Kilograms",
		.type = UNIT_MASS,
		.isPrimary = true,
		.toPrimary = NULL,
		.fromPrimary = NULL,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "mg",
		.fullName = "Miligrams",
		.type = UNIT_MASS,
		.isPrimary = false,
		.toPrimary = MgToKg,
		.fromPrimary = KgToMg,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cg",
		.fullName = "Centigrams",
		.type = UNIT_MASS,
		.isPrimary = false,
		.toPrimary = CgToKg,
		.fromPrimary = KgToCg,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "dg",
		.fullName = "Decagrams",
		.type = UNIT_MASS,
		.isPrimary = false,
		.toPrimary = DgToKg,
		.fromPrimary = KgToDg,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "g",
		.fullName = "Grams",
		.type = UNIT_MASS,
		.isPrimary = false,
		.toPrimary = GToKg,
		.fromPrimary = KgToG,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "tonne",
		.fullName = "Metric Tonnes",
		.type = UNIT_MASS,
		.isPrimary = false,
		.toPrimary = MTonneToKg,
		.fromPrimary = KgToMTonne,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "lb",
		.fullName = "Pounds",
		.type = UNIT_MASS,
		.isPrimary = false,
		.toPrimary = PoundToKg,
		.fromPrimary = KgToPound,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "oz",
		.fullName = "Ounces",
		.type = UNIT_MASS,
		.isPrimary = false,
		.toPrimary = OzToKg,
		.fromPrimary = KgToOz,
	});
	// End: Weight & mass

	// UNIT: Temperature
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "C",
		.fullName = "Celsius",
		.type = UNIT_TEMP,
		.isPrimary = false,
		.toPrimary = CelsiusToKelvin,
		.fromPrimary = KelvinToCelsius,
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "F",
		.fullName = "Fahrenheit",
		.type = UNIT_TEMP,
		.isPrimary = false,
		.toPrimary = FahrenheitToKelvin,
		.fromPrimary = KelvinToFahrenheit
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "K",
		.fullName = "Kelvin",
		.type = UNIT_TEMP,
		.isPrimary = true,
		.toPrimary = NULL, 
		.fromPrimary = NULL
	});
	// End: Temperature

	// UNIT: Energy
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "J",
		.fullName = "Joule",
		.type = UNIT_ENERGY,
		.isPrimary = true,
		.toPrimary = NULL,
		.fromPrimary = NULL
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "Ev",
		.fullName = "Electron Volts",
		.type = UNIT_ENERGY,
		.isPrimary = false,
		.toPrimary = EVToJoule,
		.fromPrimary = JouleToEv
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cal",
		.fullName = "Food calories",
		.type = UNIT_ENERGY,
		.isPrimary = false,
		.toPrimary = CalToJoule,
		.fromPrimary = JouleToCal
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "kcal",
		.fullName = "Kilo calories",
		.type = UNIT_ENERGY,
		.isPrimary = false,
		.toPrimary = KcalToJoule,
		.fromPrimary = JouleToKcal
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "Kj",
		.fullName = "Kilojoules",
		.type = UNIT_ENERGY,
		.isPrimary = false,
		.toPrimary = KjouleToJoule,
		.fromPrimary = JouleToKjoule
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "kwh",
		.fullName = "Kilowatt-hours",
		.type = UNIT_ENERGY,
		.isPrimary = false,
		.toPrimary = KwhToJoule,
		.fromPrimary = JouleToKwh
	});
	// End: Energy

	// UNIT: Area
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "sq_m",
		.fullName = "Square meter",
		.type = UNIT_AREA,
		.isPrimary = true,
		.toPrimary = NULL,
		.fromPrimary = NULL
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "sq_mm",
		.fullName = "Square millimeter",
		.type = UNIT_AREA,
		.isPrimary = false,
		.toPrimary = SqMmToSqM,
		.fromPrimary = SqMToSqMm
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "sq_cm",
		.fullName = "Square centimeter",
		.type = UNIT_AREA,
		.isPrimary = false,
		.toPrimary = SqCmToSqM,
		.fromPrimary = SqMToSqCm
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "ha",
		.fullName = "Hectare",
		.type = UNIT_AREA,
		.isPrimary = false,
		.toPrimary = HectareToSqM,
		.fromPrimary = SqMToHectare
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "sq_km",
		.fullName = "Square Kilometer",
		.type = UNIT_AREA,
		.isPrimary = false,
		.toPrimary = SqKmToSqM,
		.fromPrimary = SqMToSqKm
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "sq_inch",
		.fullName = "Square Inch",
		.type = UNIT_AREA,
		.isPrimary = false,
		.toPrimary = SqInchToSqM,
		.fromPrimary = SqMToSqInch
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "sq_feet",
		.fullName = "Square feet",
		.type = UNIT_AREA,
		.isPrimary = false,
		.toPrimary = SqFeetToSqM,
		.fromPrimary = SqMToSqFeet
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "acre",
		.fullName = "Acre",
		.type = UNIT_AREA,
		.isPrimary = false,
		.toPrimary = AcreToSqM,
		.fromPrimary = SqMToAcre
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "sq_mile",
		.fullName = "Square Miles",
		.type = UNIT_AREA,
		.isPrimary = false,
		.toPrimary = SqMileToSqM,
		.fromPrimary = SqMToSqMile
	});
	// End: Area

	// UNIT: Speed
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "mps",
		.fullName = "Meters per second",
		.type = UNIT_SPEED,
		.isPrimary = true,
		.toPrimary = NULL,
		.fromPrimary = NULL
	});
	
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "kmph",
		.fullName = "Kilometers per hour",
		.type = UNIT_SPEED,
		.isPrimary = false,
		.toPrimary = KmphToMps,
		.fromPrimary = MpsToKmph
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "mph",
		.fullName = "Miles per hour",
		.type = UNIT_SPEED,
		.isPrimary = false,
		.toPrimary = MphToMps,
		.fromPrimary = MpsToMph
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "kn",
		.fullName = "Knots",
		.type = UNIT_SPEED,
		.isPrimary = false,
		.toPrimary = KnToMps,
		.fromPrimary = MpsToKn
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cps",
		.fullName = "Centimeters per second",
		.type = UNIT_SPEED,
		.isPrimary = false,
		.toPrimary = CpsToMps,
		.fromPrimary = MpsToCps
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "fps",
		.fullName = "Feet per second",
		.type = UNIT_SPEED,
		.isPrimary = false,
		.toPrimary = FpsToMps,
		.fromPrimary = MpsToFps
	});
	// End: Speed

	// UNIT: Time
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "sec",
		.fullName = "Seconds",
		.type = UNIT_TIME,
		.isPrimary = true,
		.toPrimary = NULL,
		.fromPrimary = NULL
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "micro_sec",
		.fullName = "Microseconds",
		.type = UNIT_TIME,
		.isPrimary = false,
		.toPrimary = MicrosecToSec,
		.fromPrimary = SecToMicrosec
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "ms",
		.fullName = "Milliseconds",
		.type = UNIT_TIME,
		.isPrimary = false,
		.toPrimary = MillisecToSec,
		.fromPrimary = SecToMillisec
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "min",
		.fullName = "Minutes",
		.type = UNIT_TIME,
		.isPrimary = false,
		.toPrimary = MinuteToSec,
		.fromPrimary = SecToMinute
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "hour",
		.fullName = "Hours",
		.type = UNIT_TIME,
		.isPrimary = false,
		.toPrimary = HourToSec,
		.fromPrimary = SecToHour
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "day",
		.fullName = "Days",
		.type = UNIT_TIME,
		.isPrimary = false,
		.toPrimary = DayToSec,
		.fromPrimary = SecToDay
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "week",
		.fullName = "Weeks",
		.type = UNIT_TIME,
		.isPrimary = false,
		.toPrimary = WeekToSec,
		.fromPrimary = SecToWeek
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "year",
		.fullName = "Years",
		.type = UNIT_TIME,
		.isPrimary = false,
		.toPrimary = YearToSec,
		.fromPrimary = SecToYear
	});
	// End: Time

	// Unit: Power
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "W",
		.fullName = "Watts",
		.type = UNIT_POWER,
		.isPrimary = true,
		.toPrimary = NULL,
		.fromPrimary = NULL
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "Kw",
		.fullName = "Kilowatts",
		.type = UNIT_POWER,
		.isPrimary = true,
		.toPrimary = KwToWatt,
		.fromPrimary = WattToKw
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "Hp",
		.fullName = "Horsepower (US)",
		.type = UNIT_POWER,
		.isPrimary = true,
		.toPrimary = HpToWatt,
		.fromPrimary = WattToHp
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "Ps",
		.fullName = "Metric Horsepower",
		.type = UNIT_POWER,
		.isPrimary = true,
		.toPrimary = PsToWatt,
		.fromPrimary = WattToPs
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "btu_per_min",
		.fullName = "BTU / minute",
		.type = UNIT_POWER,
		.isPrimary = true,
		.toPrimary = BTUPerMinToWatt,
		.fromPrimary = WattToBTUPerMin
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "cal_per_sec",
		.fullName = "Calorie per second",
		.type = UNIT_POWER,
		.isPrimary = true,
		.toPrimary = CalPerSecToWatt,
		.fromPrimary = WattToCalPerSec
	});
	// End: Power

	// UNIT: Pressure
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "Pa",
		.fullName = "Pascals",
		.type = UNIT_PRESSURE,
		.isPrimary = true,
		.toPrimary = NULL,
		.fromPrimary = NULL
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "atm",
		.fullName = "Atmospheres",
		.type = UNIT_PRESSURE,
		.isPrimary = false,
		.toPrimary = AtmToPa,
		.fromPrimary = PaToAtm
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "bar",
		.fullName = "Bars",
		.type = UNIT_PRESSURE,
		.isPrimary = false,
		.toPrimary = BarToPa,
		.fromPrimary = PaToBar
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "Kpa",
		.fullName = "Kilipascals",
		.type = UNIT_PRESSURE,
		.isPrimary = false,
		.toPrimary = KPaToPa,
		.fromPrimary = PaToKPa
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "mmHg",
		.fullName = "mm of mercury",
		.type = UNIT_PRESSURE,
		.isPrimary = false,
		.toPrimary = MmHgToPa,
		.fromPrimary = PaToMmHg
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "psi",
		.fullName = "Pounds per Square Inch",
		.type = UNIT_PRESSURE,
		.isPrimary = false,
		.toPrimary = PsiToPa,
		.fromPrimary = PaToPsi
	});
	// End: Pressure

	// UNIT: Angle
	hashmap_set(g_unitsTable, &(Unit) {
		.key = "rad",
		.fullName = "Radian",
		.type = UNIT_ANGLE,
		.isPrimary = true,
		.toPrimary = NULL,
		.fromPrimary = NULL
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "deg",
		.fullName = "Degree",
		.type = UNIT_ANGLE,
		.isPrimary = false,
		.toPrimary = DegToRad,
		.fromPrimary = RadToDeg
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "grad",
		.fullName = "Gradiant",
		.type = UNIT_ANGLE,
		.isPrimary = false,
		.toPrimary = GradToRad,
		.fromPrimary = RadToGrad
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "arcmin",
		.fullName = "Archminute",
		.type = UNIT_ANGLE,
		.isPrimary = false,
		.toPrimary = ArcminToRad,
		.fromPrimary = RadToArcmin
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "arcsec",
		.fullName = "Arcsecond",
		.type = UNIT_ANGLE,
		.isPrimary = false,
		.toPrimary = ArcsecToRad,
		.fromPrimary = RadToArcsec
	});

	hashmap_set(g_unitsTable, &(Unit) {
		.key = "rev",
		.fullName = "Revolution",
		.type = UNIT_ANGLE,
		.isPrimary = false,
		.toPrimary = RevToRad,
		.fromPrimary = RadToRev
	});
	// End: Angle
}
