import json
import sys
import argparse

def verify(file_expected, file_noclip):
    try:
        with open(file_expected, 'r') as f1, open(file_noclip, 'r') as f2:
            data_ref = json.load(f1)
            data_no = json.load(f2)

        if data_ref == data_no:
            print("❌ ERROR: Files are IDENTICAL!")
            print("Clipping is NOT working or the input SVG has no viewBox boundaries.")
            sys.exit(1)  # Возвращаем ошибку для автоматизации
        else:
            print("✅ SUCCESS: Files are different (Clipping is active).")
            # Можно добавить вывод разницы, если нужно отлаживать
            # print(data_ref != data_no) 

    except Exception as e:
        print(f"💥 ERROR during comparison: {e}")
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Verify that clipping changed the geometry.")
    parser.add_argument("expected", help="The gold standard (clipped) JSON file")
    parser.add_argument("noclip", help="The no-clip version JSON file")
    args = parser.parse_args()

    verify(args.expected, args.noclip)

