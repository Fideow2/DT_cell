import pygame
import math

from pygame.math import Vector2

def sigmoid(x):
    return 1 / (1 + math.exp(-x))


class Bacteria(pygame.sprite.Sprite):
    def __init__(self, genes):
        super().__init__()
        # 设置细胞的基本属性
        self.genes = genes

        # 大小属性
        self.size = genes.get('size', 10)  # 默认大小为10

        # 离心率属性 (eccentricity) - 决定细胞形状的椭圆程度
        self.eccentricity = genes.get('eccentricity', 0.3)  # 0表示圆形，接近1表示椭圆

        # 眼睛属性
        self.eye_size = genes.get('eye_size', self.size * 0.2)  # 眼睛大小
        self.eye_angle = genes.get('eye_angle', 30)  # 眼睛角度
        self.eye_position = genes.get('eye_position', (0.7, 0.4))  # 眼睛位置比例

        # 嘴巴属性
        self.mouth_size = genes.get('mouth_size', self.size * 0.3)  # 嘴巴大小
        self.mouth_curve = genes.get('mouth_curve', 0.2)  # 嘴巴曲线程度

        # 鞭毛属性
        self.flagellum_length = genes.get('flagellum_length', self.size * 0.8)  # 鞭毛长度
        self.flagellum_curve = genes.get('flagellum_curve', 0.3)  # 鞭毛曲线程度

        # 颜色属性
        self.body_color = genes.get('body_color', (173, 216, 230))  # 淡蓝色身体
        self.eye_color = genes.get('eye_color', (0, 0, 0))  # 黑色眼睛
        self.mouth_color = genes.get('mouth_color', (255, 0, 0))  # 红色嘴巴
        self.flagellum_color = genes.get('flagellum_color', (200, 200, 200))  # 灰色鞭毛

        # 创建表面
        width = int(self.size * (1 + self.eccentricity))
        height = int(self.size)
        self.image = pygame.Surface((width * 2, height * 2), pygame.SRCALPHA)
        self.rect = self.image.get_rect()

        # 初始化位置
        self.x = 400  # 屏幕中心x坐标
        self.y = 300  # 屏幕中心y坐标

        # 身体数据
        self.speed = width - height
        self.speed = self.size * sigmoid(self.speed)
        while self.speed > 5:  # 限制速度上限，避免移动过快
            self.speed *= 0.95

        self.defense = height - width
        self.defense = self.size * sigmoid(self.defense)

        self.max_hp = self.size * 10
        self.attack = self.size * 5 ** (width / height)

    def draw(self, surface, pos):
        if not self.image:
            return

        # 清除表面
        self.image.fill((0, 0, 0, 0))

        # 细胞中心
        center = (self.image.get_width() // 2, self.image.get_height() // 2)

        # 绘制椭圆形身体
        width = int(self.size * (1 + self.eccentricity))
        height = int(self.size)
        pygame.draw.ellipse(self.image, self.body_color, (center[0] - width//2, center[1] - height//2, width, height))

        # 绘制眼睛
        eye_x = center[0] + int(self.eye_position[0] * width/2 * 0.5)
        eye_y = center[1] - int(self.eye_position[1] * height/2)
        eye_width = int(self.eye_size * 1.5)  # 椭圆眼睛
        eye_height = int(self.eye_size)

        # 创建眼睛表面并旋转
        eye_surface = pygame.Surface((eye_width*2, eye_height*2), pygame.SRCALPHA)
        pygame.draw.ellipse(eye_surface, self.eye_color, (eye_width//2, eye_height//2, eye_width, eye_height))

        # 旋转眼睛
        eye_surface = pygame.transform.rotate(eye_surface, self.eye_angle)
        eye_rect = eye_surface.get_rect(center=(eye_x, eye_y))
        self.image.blit(eye_surface, eye_rect)

        # 绘制嘴巴 (贝塞尔曲线)
        mouth_start = (center[0] + width//3, center[1] + height//10)
        mouth_end = (center[0] + width//2, center[1] + height//10)
        mouth_control = (center[0] + width//2.5, center[1] + height//4)

        # 使用多个小线段近似贝塞尔曲线
        last_point = mouth_start
        for t in range(1, 21):
            t = t / 20
            # 二次贝塞尔曲线公式
            x = (1-t)**2 * mouth_start[0] + 2*(1-t)*t*mouth_control[0] + t**2*mouth_end[0]
            y = (1-t)**2 * mouth_start[1] + 2*(1-t)*t*mouth_control[1] + t**2*mouth_end[1]
            pygame.draw.line(self.image, self.mouth_color, last_point, (x, y), 2)
            last_point = (x, y)

        # 绘制鞭毛 (另一侧的曲线)
        flagellum_start = (center[0] - width//2, center[1])
        # 增加鞭毛长度，将原来的长度乘以1.5倍
        extended_length = self.flagellum_length * 1.5
        flagellum_control1 = (center[0] - width//2 - extended_length//2, center[1] - height//3)
        flagellum_control2 = (center[0] - width//2 - extended_length//1.5, center[1] + height//3)
        flagellum_end = (center[0] - width//2 - extended_length, center[1])

        # 使用多个小线段近似三次贝塞尔曲线
        last_point = flagellum_start
        for t in range(1, 21):
            t = t / 20
            # 三次贝塞尔曲线公式
            x = (1-t)**3 * flagellum_start[0] + 3*(1-t)**2*t * flagellum_control1[0] + 3*(1-t)*t**2 * flagellum_control2[0] + t**3 * flagellum_end[0]
            y = (1-t)**3 * flagellum_start[1] + 3*(1-t)**2*t * flagellum_control1[1] + 3*(1-t)*t**2 * flagellum_control2[1] + t**3 * flagellum_end[1]
            pygame.draw.line(self.image, self.flagellum_color, last_point, (x, y), 2)
            last_point = (x, y)

        # 绘制到主表面
        self.rect = self.image.get_rect(center=(self.x, self.y))
        surface.blit(self.image, self.rect)

    def update(self, keys=None):
        """根据按键输入更新细菌位置"""
        if not keys:
            return

        # 初始化运动相关属性
        if not hasattr(self, 'velocity'):
            self.velocity = Vector2(0, 0)
        if not hasattr(self, 'acceleration'):
            self.acceleration = Vector2(0, 0)
        if not hasattr(self, 'face_right'):
            self.face_right = True

        # 常量参数
        max_speed = self.speed
        acceleration_step = 1.0
        drag = 0.98  # 水阻力

        # 重置加速度
        self.acceleration = Vector2(0, 0)

        # 处理按键输入
        if keys[pygame.K_w]: self.acceleration.y -= acceleration_step  # 上
        if keys[pygame.K_s]: self.acceleration.y += acceleration_step  # 下
        if keys[pygame.K_a]:
            self.acceleration.x -= acceleration_step  # 左
            self.face_right = False  # 面向左
        if keys[pygame.K_d]:
            self.acceleration.x += acceleration_step  # 右
            self.face_right = True  # 面向右

        # 更新物理状态
        self.velocity += self.acceleration
        # 限制速度
        self.velocity.x = max(-max_speed, min(self.velocity.x, max_speed))
        self.velocity.y = max(-max_speed, min(self.velocity.y, max_speed))

        # 更新位置
        self.x += self.velocity.x
        self.y += self.velocity.y

        # 应用阻力
        self.velocity *= drag

        # 边界检查
        self.x = max(0, min(self.x, 800))  # 屏幕宽度
        self.y = max(0, min(self.y, 600))  # 屏幕高度


if __name__ == "__main__":
    pygame.init()
    screen = pygame.display.set_mode((800, 600))

    cell = Bacteria({'size':100})
    running = True
    while running:
        screen.fill((255, 255, 255))  # 清空屏幕，以便看到细胞移动
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
        cell.update(pygame.key.get_pressed())
        cell.draw(screen, (cell.x, cell.y))
        pygame.display.flip()
    pygame.quit()
