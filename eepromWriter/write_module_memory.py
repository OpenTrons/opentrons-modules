import sys

from serial import Serial
from serial.tools.list_ports import comports


OPENTRONS_VID = 1240

port = ''

BAD_BARCODE_MESSAGE = 'Unexpected Serial -> {}'
WRITE_FAIL_MESSAGE = 'Data not saved'

MODELS = {
    'TDV01': 'temp_deck_v1.1',  # make sure to skip v2 if model updates
    'MDV01': 'mag_deck_v1.1'
}


def connect_to_temp_deck():
    port = None
    for p in comports():
        if p.vid == OPENTRONS_VID:
            port = p.device
            break
    if port is None:
        raise RuntimeError('Could not find Opentrons Model connected over USB')
    print()
    print('Connecting to temp-deck...')
    td = Serial(port, 115200, timeout=1)
    return td


def write_identifiers(tempdeck, new_id, new_model):
    msg = '{0}:{1}'.format(new_id, new_model)
    tempdeck.write(msg.encode())
    tempdeck.read_until(b'\r\n')
    serial, model = _get_info(tempdeck)
    _assert_the_same(new_id, serial)
    _assert_the_same(new_model, model)


def check_previous_data(tempdeck):
    serial, model = _get_info(tempdeck)
    if not serial or not model:
        print('No old data on this pipette')
        return
    print(
        'Overwriting old data: id={0}, model={1}'.format(serial, model))


def _get_info(tempdeck):
    info = (None, None)
    tempdeck.write(b'&')  # special character to retrive old data
    res = tempdeck.read_until(b'\r\n').decode().strip()
    return tuple(res.split(':'))


def _assert_the_same(a, b):
    if a != b:
        raise Exception(WRITE_FAIL_MESSAGE + ' ({0} != {1})'.format(a, b))


def _user_submitted_barcode(max_length):
    barcode = input('SCAN: ').strip()
    if len(barcode) > max_length:
        raise Exception(BAD_BARCODE_MESSAGE.format(barcode))
    # remove all characters before the letter T
    # for example, remove ASCII selector code "\x1b(B" on chinese keyboards
    barcode = barcode[barcode.index('T'):]
    barcode = barcode.split('\n')[0].split('\r')[0]
    return barcode


def _parse_model_from_barcode(barcode):
    for barcode_substring, model in MODELS.items():
        if barcode.startswith(barcode_substring):
            return model
    raise Exception(BAD_BARCODE_MESSAGE.format(barcode))


def main():
    print('\n')
    try:
        barcode = _user_submitted_barcode(32)
        model = _parse_model_from_barcode(barcode)
        tempdeck = connect_to_temp_deck()
        check_previous_data(tempdeck)
        write_identifiers(tempdeck, barcode, model)
        print('PASS: Saved -> {0} (model {1})'.format(barcode, model))
    except KeyboardInterrupt:
        exit()
    except Exception as e:
        print('FAIL: {}'.format(e))
    main()


if __name__ == "__main__":
    main()
