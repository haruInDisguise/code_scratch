from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.support.wait import WebDriverWait
import selenium.webdriver.support.expected_conditions as expected_conditions
import subprocess

def main():
    service = webdriver.FirefoxService(log_output=subprocess.STDOUT, service_args=['--log', 'debug', '--profile-root', '/home/haru/.mozilla/firefox'])
    options = webdriver.FirefoxOptions()
    options.add_argument('-profile')
    options.add_argument('/home/haru/.mozilla/firefox/webdriver')

    driver = webdriver.Firefox(options=options, service=service)
    wait = WebDriverWait(driver, timeout=5)
    action = ActionChains(driver)

    driver.implicitly_wait(30)
    driver.get("https://orteil.dashnet.org/cookieclicker/")

    # Check of language selection dialog
    driver.find_element(by=By.CSS_SELECTOR, value='#langSelect-EN').click()

    # Locate cookie and click it
    element_cookie = driver.find_element(by=By.CSS_SELECTOR, value='#bigCookie')
    wait.until(expected_conditions.element_to_be_clickable(element_cookie))

    while True:
        action.move_to_element(element_cookie).click().pause(.25).perform()

    driver.quit()

if __name__ == '__main__':
    main()
