// MoistureSensor.js
import React, { useEffect } from 'react';
import { LinearGauge, Title, Font, Geometry, Scale, RangeContainer, ValueIndicator } from 'devextreme-react/linear-gauge';
import "./MoistureSensor.css";

const MoistureSensor = ({ sensorData }) => {
  useEffect(() => {
    // console.log("SENSOR: ", sensorData);
  }, [sensorData]);

  const isSoilMoist = sensorData > 70;

  return (
    <div className='moisture-sensor-container'>
      <div className='moisture-sensor'>
        <LinearGauge
          className="gauge-element"
          value={sensorData}
        >
          <Title text="Umidade (%)" style={{ fontSize: '16px', color: '#ADD8E6' }} >
            <Font size={12} />
          </Title>
          <Geometry orientation="vertical" />
          <Scale startValue={0} endValue={100} tickInterval={10} />
          <RangeContainer backgroundColor="#CACACA" />
          <ValueIndicator type="rhombus" color="#A4DDED" />
        </LinearGauge>
      </div>
      <div className={`soil-status ${isSoilMoist ? 'moist' : 'dry'}`}>
        {isSoilMoist ? 'Solo Úmido' : 'Solo Seco'}
      </div>
    </div>
  );
};

export default MoistureSensor;
