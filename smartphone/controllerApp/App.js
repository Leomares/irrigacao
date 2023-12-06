import React, { useState } from 'react';
import {
  Button,
  View,
  Text,
  FlatList,
  StyleSheet,
  TextInput,
  TouchableOpacity,
  ScrollView,
} from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';

import { Picker } from '@react-native-picker/picker';
import base64 from 'react-native-base64'

import { BLEService } from '.'
import { fullUUID, Device} from 'react-native-ble-plx'

function ButtonsScreen({ navigation }) {
  return (
    <View
      style={{
        flex: 1,
        alignItems: 'center',
        flexDirection: 'row',
        justifyContent: 'center',
      }}>
      <Button
        title="Go to Home"
        onPress={() => navigation.navigate('Device')}
      />
      <Button title="Go back" onPress={() => navigation.navigate('Profile')} />
    </View>
  );
}
  

const Item = ({ item, onPress, backgroundColor, textColor }) => (
  <TouchableOpacity
    onPress={onPress}
    style={[styles.item, { backgroundColor }]}>
    <View
      style={{
        flex: 1,
        alignItems: 'left',
        flexDirection: 'row',
        justifyContent: 'left',
        backgroundColor: { backgroundColor },
      }}>
      <Text style={[styles.title, { textAlign: 'left' }, { color: textColor }]}>
        {item.title}
      </Text>
      <View
        style={{
          flex: 1,
          alignItems: 'center',
          justifyContent: 'center',
          alignSelf: 'stretch',
          margin: 5,
        }}
      />
      <Text
        style={[styles.title, { textAlign: 'right' }, { color: textColor }]}>
        {item.db_value}
      </Text>
    </View>
  </TouchableOpacity>
);

function BLEScreen({ navigation }) {
  React.useEffect(() => {
    navigation.setOptions({
      headerLeft: () => (
        <Button
        onPress={() => navigation.navigate('Device')}
        title="Device"
        color= "#2196F3"
        />
      ),
      headerRight: () => (
        <View
      style={{
        flexDirection: 'row',
      }}>
        <Button
          onPress={() => navigation.navigate('Wi-Fi')}
          title="Wi-Fi"
          color= "#444444"
        />
        <Button
          onPress={() => navigation.navigate('Profile')}
          title="Profile"
          color= "#444444"
        />
        </View>
      ),
    });
  }, [navigation]);

  const [isIdle, setIsIdle] = useState(true)
  const [foundDevices, setFoundDevices] = useState([])//<Device[]>([])

  const addFoundDevice = (device: Device) =>
    setFoundDevices(prevState => {
      if (!isFoundDeviceUpdateNecessary(prevState, device)) {
        return prevState
      }
      // deep clone
      const nextState = cloneDeep(prevState)
      return nextState.concat(extendedDevice)
    })

  const isFoundDeviceUpdateNecessary = (currentDevices: Device[], updatedDevice: Device) => {
    const currentDevice = currentDevices.find(({ id }) => updatedDevice.id === id)
    if(!currentDevice){return true}
    return false
  }

  const onConnectSuccess = () => {
    setIsIdle(true);
    navigation.navigate('Profile');
    BLEService.discoverAllServicesAndCharacteristicsForDevice();
  }

  const onConnectFail = () => {
    setIsIdle(true)
  }

  const deviceRender = (device: Device) => {
    const backgroundColor = device.id === BLEService.device.id ? '#222222' : '#dddddd';
    const color = device.id === BLEService.device.id ? 'white' : 'black';

    return (
      <TouchableOpacity
      onPress={pickedDevice => {
      BLEService.connectToDevice(pickedDevice.id).then(onConnectSuccess).catch(onConnectFail)}}
      style={[styles.item, { backgroundColor }]}>
        <View
        style={{
          flex: 1,
          alignItems: 'left',
          flexDirection: 'row',
          justifyContent: 'left',
          backgroundColor: { backgroundColor },
        }}>
          <Text style={[styles.title, { textAlign: 'left' }, { color: color }]}>
            {device.rssi}
          </Text>
          <View
          style={{
            flex: 1,
            alignItems: 'center',
            justifyContent: 'center',
            alignSelf: 'stretch',
            margin: 5,
          }}
          />
          <Text style={[styles.title, { textAlign: 'right' }, { color: color }]}>
            {device.rssi}
          </Text>
        </View>
      </TouchableOpacity>
    )
  };

  return (
    <View
      style={{ flex: 1, padding: 0, marginVertical: 0, marginHorizontal: 0 }}>
      <View
        style={{
          flex: 1,
          padding: 8,
          marginVertical: 26,
          marginHorizontal: 10,
        }}>
        <Button onPress={() => {setFoundDevices([]);
          setIsIdle(false);
          BLEService.initializeBLE().then(() => 
          BLEService.scanDevices(addFoundDevice, null, true));}} title= {isIdle ? "Scan" : "Scanning"} 
          disabled={!isIdle}
          />
      </View>
      <FlatList
        data={foundDevices}
        renderItem={deviceRender}
        keyExtractor={(device) => device.id}
      />
    </View>
  );
}


const S_WIFI_UUID = fullUUID("2a38798e-8a3e-11ee-b9d1-0242ac120002");

// Read whether the microcontroller is connected to a WiFi network.
//TODO: extra characteristic to read ssid?
const C_WIFI_R_STATUS_UUID = fullUUID("2a387d08-8a3e-11ee-b9d1-0242ac120002");

// Set SSID and password to the microcontroller.
const C_WIFI_W_CONFIG_UUID = fullUUID("2a387bdc-8a3e-11ee-b9d1-0242ac120002");


function WifiScreen({ navigation }) {
  React.useEffect(() => {
    navigation.setOptions({
      headerLeft: () => (
        <Button
        onPress={() => navigation.navigate('Device')}
        title="Device"
        color= "#444444"
        />
      ),
      headerRight: () => (
        <View
      style={{
        flexDirection: 'row',
      }}>
        <Button
          onPress={() => navigation.navigate('Wi-Fi')}
          title="Wi-Fi"
          color= "#2196F3"
        />
        <Button
          onPress={() => navigation.navigate('Profile')}
          title="Profile"
          color= "#444444"
        />
        </View>
      ),
    });
    {
    const unsubscribe = navigation.addListener('focus', () => {
    WifiScreen_read_ssid();
    });

    // Return the function to unsubscribe from the event so it gets removed on unmount
    return unsubscribe;
    }
  }, [navigation]);

  const [ssid, setSsid] = useState('');
  const [password, setPassword] = useState('');

  const [concatRes, setConcatRes] = useState('');
  const [concatRes64, setconcatRes64] = useState('');

  function WifiScreen_read_ssid_and_status(){
    const readCharacteristic = BLEService.readCharacteristicForDevice(S_WIFI_UUID, C_WIFI_R_STATUS_UUID) TODO: extra characteristic?
    const wifi_ssid = base64.decode(readCharacteristic.value);
    setSsid(wifi_ssid);
  }
  
  function WifiScreen_write_ssid_password() {
    const payload_str = ssid + ',' + password;
    setConcatRes(payload_str);
    const payload_64 = base64.encode(payload_str);
    setconcatRes64(payload_64);
    BLEService.writeCharacteristicWithResponseForDevice(S_WIFI_UUID, C_WIFI_W_CONFIG_UUID,payload_64)
  }

  return (
    <View style={{ padding: 10, marginVertical: 0, marginHorizontal: 10 }}>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 60,
        }}>
        <Text style={{ fontSize: 20 }}>Wi-Fi name</Text>
        <TextInput
          style={{
            height: 30,
            fontSize: 20,
            borderWidth: 1,
            backgroundColor: '#ffffff',
          }}
          onChangeText={(newText) => setSsid(newText)}
          value={ssid}
        />
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 70,
        }}>
        <Text style={{ fontSize: 20 }}>Wi-Fi password</Text>
        <TextInput
          style={{
            height: 30,
            fontSize: 20,
            borderWidth: 1,
            backgroundColor: '#ffffff',
          }}
          onChangeText={(newText) => setPassword(newText)}
          value={password}
        />
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 50,
        }}>
        <Button
          onPress={() =>{ WifiScreen_write_ssid_password();}}
          title="Apply configuration"
        />
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 50,
        }}>
        <Text>{ssid}</Text>
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 50,
        }}>
        <Text>{password}</Text>
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 50,
        }}>
        <Text>{concatRes}</Text>
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 50,
        }}>
        <Text>{concatRes64}</Text>
      </View>
    </View>
  );
}



var standardProfiles = {
  "default": {
    "outside": "1",
    "volume": "200",
    "regularPeriod": "0",
    "cooldownPeriod": "1"
  },
  "a": {
    "outside": "1",
    "volume": "321",
    "regularPeriod": "963",
    "cooldownPeriod": "741"
  },
  "b": {
    "outside": "0",
    "volume": "842",
    "regularPeriod": "426",
    "cooldownPeriod": "839"
  }
}

const S_PROFILE_UUID = fullUUID("4e0b4756-8a3e-11ee-b9d1-0242ac120002");

/*
 set a profile in a certain index. Serialization used:
    "int index,bool isOutside,int volume,int regularPeriod,int cooldownPeriod"
*/
const C_PROFILE_W_UUID = fullUUID("4e0b4a62-8a3e-11ee-b9d1-0242ac120002");
// TODO: use this or C_CONTROL_R_W_PROFILE_UUID ?

const S_CONTROL_UUID = {
    "1": fullUUID("8f904730-8a3e-11ee-b9d1-0242ac120002"),
    "2": fullUUID("d6b23984-8a3e-11ee-b9d1-0242ac120002"),
    "3": fullUUID("d6b23ee8-8a3e-11ee-b9d1-0242ac120002"),
    "4": fullUUID("d6b24050-8a3e-11ee-b9d1-0242ac120002")};

// characteristic to read and write whether a controller is enable or not.
const C_CONTROL_R_W_STATUS_UUID = {
    "1": fullUUID("8f904b86-8a3e-11ee-b9d1-0242ac120002"),
    "2": fullUUID("c6a16c68-8a3e-11ee-b9d1-0242ac120002"),
    "3": fullUUID("c6a1700a-8a3e-11ee-b9d1-0242ac120002"),
    "4": fullUUID("c6a1719a-8a3e-11ee-b9d1-0242ac120002")};

// characteristic to read the current profile of a controller.
const C_CONTROL_R_W_PROFILE_UUID = {
    "1": fullUUID("817bf150-8eec-11ee-b9d1-0242ac120002"),
    "2": fullUUID("817bf39e-8eec-11ee-b9d1-0242ac120002"),
    "3": fullUUID("817bf4b6-8eec-11ee-b9d1-0242ac120002"),
    "4": fullUUID("817bf5ba-8eec-11ee-b9d1-0242ac120002")};


function ProfileScreen({ navigation }) {
  React.useEffect(() => {
    navigation.setOptions({
      headerLeft: () => (
        <Button
        onPress={() => navigation.navigate('Device')}
        title="Device"
        color= "#444444"
        />
      ),
      headerRight: () => (
        <View
      style={{
        flexDirection: 'row',
      }}>
        <Button
          onPress={() => navigation.navigate('Wi-Fi')}
          title="Wi-Fi"
          color= "#444444"
        />
        <Button
          onPress={() => navigation.navigate('Profile')}
          title="Profile"
          color= "#2196F3"
        />
        </View>
      ),
    });
    {
    const unsubscribe = navigation.addListener('focus', () => {
    profile_handler("1");
    });

    // Return the function to unsubscribe from the event so it gets removed on unmount
    return unsubscribe;
    }
  }, [navigation]);

  const [selectedProfile, setSelectedProfile] = useState("1");
  const [selectedStandard, setSelectedStandard] = useState("current");
  const [volume, setVolume] = useState("200");
  const [regularPeriod, setRegularPeriod] = useState("0");
  const [cooldownPeriod, setCooldownPeriod] = useState("1");
  const [isOutside, setIsOutside] = useState("1");
  const [enabled, setEnabled] = useState("0");

  const [concatRes, setConcatRes] = useState("");
  const [concatRes64, setconcatRes64] = useState("");

  function ProfileScreen_read_current_profile(){
    const readCharacteristic = BLEService.readCharacteristicForDevice(S_CONTROL_UUID[selectedProfile], C_CONTROL_R_W_PROFILE_UUID[selectedProfile])
    const profile_info = base64.decode(readCharacteristic.value);
    setIsOutside(profile_info.substring(0, 1));
    setVolume(profile_info.substring(1, 4));
    setRegularPeriod(profile_info.substring(4, 7));
    setCooldownPeriod(profile_info.substring(7, 10));
    setConcatRes(profile_info.substring(0, 1));
    ProfileScreen_read_enabled();
  }
  
  function ProfileScreen_write_profile() {
    const payload_str = isOutside + "0".repeat(3-volume.length) + volume + "0".repeat(3-regularPeriod.length) + regularPeriod + "0".repeat(3-cooldownPeriod.length) + cooldownPeriod;
    setConcatRes(payload_str);
    const payload_64 = base64.encode(payload_str);
    setconcatRes64(payload_64);
    BLEService.writeCharacteristicWithResponseForDevice(S_CONTROL_UUID[selectedProfile], C_CONTROL_R_W_PROFILE_UUID[selectedProfile],payload_64)
  }

  function ProfileScreen_read_enabled(){
    const readCharacteristic = BLEService.readCharacteristicForDevice(S_CONTROL_UUID[selectedProfile], C_CONTROL_R_W_STATUS_UUID[selectedProfile])
    setEnabled(base64.decode(readCharacteristic.value));
  }
  
  function ProfileScreen_write_enabled() {
    const payload_64 = base64.encode("1");
    setconcatRes64(payload_64);
    BLEService.writeCharacteristicWithResponseForDevice(S_CONTROL_UUID[selectedProfile], C_CONTROL_R_W_STATUS_UUID[selectedProfile],payload_64)
    ProfileScreen_read_enabled();
  }

  function profile_handler(selected_profile) {
    setSelectedProfile(selected_profile);
    setSelectedStandard("current");
    ProfileScreen_read_current_profile();
    ProfileScreen_read_enabled()
  }

  function standard_handler(selected_standard) {
    setSelectedStandard(selected_standard);
    if(selected_standard === "current"){
      ProfileScreen_read_current_profile();
    }
    else{
      var profile_info = standardProfiles[selected_standard]
      
      setIsOutside(profile_info["outside"]);
      setVolume(profile_info["volume"]);
      setRegularPeriod(profile_info["regularPeriod"]);
      setCooldownPeriod(profile_info["cooldownPeriod"]);
    }
  }

  return (
    <ScrollView
      style={{ padding: 10, marginVertical: 0, marginHorizontal: 10 }}>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 40,
        }}>
        <Picker
          selectedValue={selectedProfile}
          onValueChange={(itemValue, itemIndex) => profile_handler(itemValue)}
          style={[{ fontSize: 20 }, { height: 30 }]}>
          <Picker.Item label="Profile 1" value="1" />
          <Picker.Item label="Profile 2" value="2" />
          <Picker.Item label="Profile 3" value="3" />
          <Picker.Item label="Profile 4" value="4" />
        </Picker>
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 40,
        }}>
        <Picker
          selectedValue={selectedStandard}
          onValueChange={(itemValue, itemIndex) =>
            standard_handler(itemValue)
          }
          style={[{ fontSize: 20 }, { height: 30 }]}>
          <Picker.Item label="Current" value="current" />
          <Picker.Item label="Default" value="default" />
          <Picker.Item label="Plant A" value="a" />
          <Picker.Item label="Plant B" value="b" />
        </Picker>
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 60,
        }}>
        <Text style={{ fontSize: 20 }}>Volume (ml)</Text>
        <TextInput
          style={{
            height: 30,
            fontSize: 20,
            borderWidth: 1,
            backgroundColor: '#ffffff',
          }}
          placeholder="Irrigation volume"
          onChangeText={(newText) => setVolume(newText.substring(newText.length-3,newText.length))}
          value={volume}
          keyboardType="numeric"
        />
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 60,
        }}>
        <Text style={{ fontSize: 20 }}>Regular period (s)</Text>
        <TextInput
          style={{
            height: 30,
            fontSize: 20,
            borderWidth: 1,
            backgroundColor: '#ffffff',
          }}
          placeholder="Regular period"
          onChangeText={(newText) => setRegularPeriod(newText.substring(newText.length-3,newText.length))}
          value={regularPeriod}
          keyboardType="numeric"
        />
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 60,
        }}>
        <Text style={{ fontSize: 20 }}>Cooldown period (s)</Text>
        <TextInput
          style={{
            height: 30,
            fontSize: 20,
            borderWidth: 1,
            backgroundColor: '#ffffff',
          }}
          placeholder="Cooldown period"
          onChangeText={(newText) => setCooldownPeriod(newText.substring(newText.length-3,newText.length))}
          value={cooldownPeriod}
          keyboardType="numeric"
        />
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 70,
        }}>
        <Text style={{ fontSize: 20 }}>Location</Text>
        <Picker
          selectedValue={isOutside}
          onValueChange={(itemValue, itemIndex) => setIsOutside(itemValue)}
          style={{ fontSize: 20, height: 30 }}>
          <Picker.Item label="Outside" value="1" />
          <Picker.Item label="Inside" value="0" />
        </Picker>
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 50,
        }}>
        <Button
          onPress={() => {ProfileScreen_write_profile();
          }}
          title="Apply configuration"
        />
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 40,
        }}>
        <Button
          onPress={() => {
            ProfileScreen_write_enabled();
          }}
          disabled={enabled === "1"}
          title={enabled === "0" ? 'Enable profile' : 'Profile enabled'}
        />
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 50,
        }}>
        <Text>{concatRes}</Text>
      </View>
      <View
        style={{
          padding: 0,
          marginVertical: 0,
          marginHorizontal: 0,
          height: 50,
        }}>
        <Text>{concatRes64}</Text>
      </View>
    </ScrollView>
  );
}

const Stack = createNativeStackNavigator();

function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator initialRouteName="Profile">
        <Stack.Screen name="Device" component={BLEScreen} />
        <Stack.Screen name="Wi-Fi" component={WifiScreen} />
        <Stack.Screen name="Profile" component={ProfileScreen} />
      </Stack.Navigator>
    </NavigationContainer>
  );
}

const styles = StyleSheet.create({
  item: {
    padding: 20,
    marginVertical: 8,
    marginHorizontal: 16,
  },
  title: {
    fontSize: 15,
  },
});

export default App;
